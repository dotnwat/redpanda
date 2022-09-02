/*
 * Copyright 2020 Redpanda Data, Inc.
 *
 * Use of this software is governed by the Business Source License
 * included in the file licenses/BSL.md
 *
 * As of the Change Date specified in that file, in accordance with
 * the Business Source License, use of this software will be governed
 * by the Apache License, Version 2.0
 */
#include "security/scram_authenticator.h"

#include "bytes/bytes.h"
#include "kafka/protocol/request_reader.h"
#include "security/errc.h"
#include "security/logger.h"
#include "vlog.h"

#include <gssapi/gssapi.h>

namespace security {

template<typename T>
result<bytes>
scram_authenticator<T>::handle_client_first(bytes_view auth_bytes) {
    // request from client
    _client_first = std::make_unique<client_first_message>(auth_bytes);
    vlog(seclog.debug, "Received client first message {}", *_client_first);

    // lookup credentials for this user
    _authid = _client_first->username_normalized();
    auto credential = _credentials.get<scram_credential>(
      credential_user(_authid));
    if (!credential) {
        return errc::invalid_credentials;
    }
    _credential = std::make_unique<scram_credential>(std::move(*credential));

    if (
      !_client_first->authzid().empty()
      && _client_first->authzid() != _authid) {
        vlog(seclog.info, "Invalid authorization id and username pair");
        return errc::invalid_credentials;
    }

    if (_credential->iterations() < scram::min_iterations) {
        vlog(
          seclog.info,
          "Requested iterations less than minimum {}",
          scram::min_iterations);
        return errc::invalid_credentials;
    }

    // build server reply
    _server_first = std::make_unique<server_first_message>(
      _client_first->nonce(),
      random_generators::gen_alphanum_string(nonce_size),
      _credential->salt(),
      _credential->iterations());

    _state = state::client_final_message;

    auto reply = _server_first->sasl_message();
    return bytes(reply.cbegin(), reply.cend());
}

template<typename T>
result<bytes>
scram_authenticator<T>::handle_client_final(bytes_view auth_bytes) {
    client_final_message client_final(auth_bytes);
    vlog(seclog.debug, "Received client final message {}", client_final);

    auto client_signature = scram::client_signature(
      _credential->stored_key(), *_client_first, *_server_first, client_final);

    auto computed_stored_key = scram::computed_stored_key(
      client_signature,
      bytes(client_final.proof().begin(), client_final.proof().end()));

    if (computed_stored_key != _credential->stored_key()) {
        vlog(
          seclog.info,
          "Authentication failed: stored and client submitted credentials do "
          "not match");
        return errc::invalid_credentials;
    }

    const auto username = _client_first->username();
    vlog(seclog.debug, "Authentication key match for user {}", username);

    auto server_signature = scram::server_signature(
      _credential->server_key(), *_client_first, *_server_first, client_final);

    server_final_message server_final(std::nullopt, server_signature);
    auto reply = server_final.sasl_message();

    clear_credentials();
    _state = state::complete;

    return bytes(reply.cbegin(), reply.cend());
}

template<typename T>
void scram_authenticator<T>::clear_credentials() {
    _credential.reset();
    _client_first.reset();
    _server_first.reset();
}

template<typename T>
result<bytes> scram_authenticator<T>::handle_next(bytes_view auth_bytes) {
    switch (_state) {
    case state::client_first_message:
        return handle_client_first(auth_bytes);

    case state::client_final_message:
        return handle_client_final(auth_bytes);

    default:
        return errc::invalid_scram_state;
    }
}

template<typename T>
result<bytes> scram_authenticator<T>::authenticate(bytes_view auth_bytes) {
    auto ret = handle_next(auth_bytes);
    if (!ret) {
        _state = state::failed;
        clear_credentials();
    }
    return ret;
}

template class scram_authenticator<scram_sha256>;
template class scram_authenticator<scram_sha512>;

static void display_status_1(const char* m, OM_uint32 code, int type) {
    OM_uint32 maj_stat, min_stat;
    gss_buffer_desc msg = GSS_C_EMPTY_BUFFER;
    OM_uint32 msg_ctx;

    msg_ctx = 0;
    while (1) {
        maj_stat = gss_display_status(
          &min_stat, code, type, GSS_C_NO_OID, &msg_ctx, &msg);
        if (maj_stat != GSS_S_COMPLETE) {
            vlog(seclog.info, "gss status from {}", m);
            break;
        } else {
            vlog(seclog.info, "GSS_API error {}: {}", m, (char*)msg.value);
        }

        if (msg.length != 0) {
            (void)gss_release_buffer(&min_stat, &msg);
        }

        if (!msg_ctx) {
            break;
        }
    }
}

/*
 * Function: display_status
 *
 * Purpose: displays GSS-API messages
 *
 * Arguments:
 *
 *      msg             a string to be displayed with the message
 *      maj_stat        the GSS-API major status code
 *      min_stat        the GSS-API minor status code
 *
 * Effects:
 *
 * The GSS-API messages associated with maj_stat and min_stat are
 * displayed on stderr, each preceeded by "GSS-API error <msg>:
" and
 * followed by a newline.
 */
void display_status(const char* msg, OM_uint32 maj_stat, OM_uint32 min_stat) {
    display_status_1(msg, maj_stat, GSS_C_GSS_CODE);
    display_status_1(msg, min_stat, GSS_C_MECH_CODE);
}

result<bytes> gssapi_authenticator::authenticate(bytes_view auth_bytes) {
    gss_name_t service_name;
    OM_uint32 minor_status;
    OM_uint32 major_status;
    gss_cred_id_t server_creds;
    gss_buffer_desc server_name;

    std::string service = "kafka";
    server_name.value = service.data();
    server_name.length = service.size();

    major_status = ::gss_import_name(
      &minor_status, &server_name, GSS_C_NT_HOSTBASED_SERVICE, &service_name);

    vlog(seclog.info, "gss import name {}:{}", major_status, minor_status);
    if (major_status != GSS_S_COMPLETE) {
        display_status("import name", major_status, minor_status);
        return errc::invalid_scram_state;
    }

    // seems to have read from file
    major_status = ::gss_acquire_cred(
      &minor_status,
      service_name,
      0,
      GSS_C_NO_OID_SET,
      GSS_C_ACCEPT,
      &server_creds,
      NULL,
      NULL);

    vlog(seclog.info, "gss acquire cred {}:{}", major_status, minor_status);
    if (major_status != GSS_S_COMPLETE) {
        display_status("gss acquire cred", major_status, minor_status);
        return errc::invalid_scram_state;
    }

    gss_ctx_id_t context = GSS_C_NO_CONTEXT;

    gss_buffer_desc recv_tok, send_tok;
    recv_tok.value = (void*)auth_bytes.data();
    recv_tok.length = auth_bytes.length();

    gss_name_t client;
    gss_OID doid;
    OM_uint32 ret_flags;

    major_status = ::gss_accept_sec_context(
      &minor_status,
      &context,
      server_creds,
      &recv_tok,
      GSS_C_NO_CHANNEL_BINDINGS,
      &client,
      &doid,
      &send_tok,
      &ret_flags,
      NULL,  /* ignore time_rec */
      NULL); /* ignore del_cred_handle */

    vlog(
      seclog.info, "gss acccept sec context {}:{}", major_status, minor_status);
    if (major_status != GSS_S_COMPLETE) {
        display_status("gss accept sec context", major_status, minor_status);
        return errc::invalid_scram_state;
    }

    bytes res((unsigned char*)send_tok.value, send_tok.length);
    res[res.size()/2] = 'a';
    res[(res.size()/2)+1] = 'b';
    return res;

    // vlog(seclog.info, "XXXXXXXXXX: {}", to_hex(auth_bytes));
    return errc::invalid_scram_state;
}

#if 0

/*
 * Function: server_acquire_creds
 *
 * Purpose: imports a service name and acquires credentials for it
 *
 * Arguments:
 *
 *      service_name    (r) the ASCII service name
        mechType        (r) the mechanism type to use
 *      server_creds    (w) the GSS-API service credentials
 *
 * Returns: 0 on success, -1 on failure
 *
 * Effects:
 *
 * The service name is imported with gss_import_name, and service
 * credentials are acquired with gss_acquire_cred.  If either operation
 * fails, an error message is displayed and -1 is returned; otherwise,
 * 0 is returned.
 */
int server_acquire_creds(service_name, mechOid, server_creds)
     char *service_name;
     gss_OID mechOid;
     gss_cred_id_t *server_creds;
{
     gss_buffer_desc name_buf;
     gss_name_t server_name;
     OM_uint32 maj_stat, min_stat;
     gss_OID_set_desc mechOidSet;
     gss_OID_set desiredMechs = GSS_C_NULL_OID_SET;

     if (mechOid != GSS_C_NULL_OID) {
                desiredMechs = &mechOidSet;
                mechOidSet.count = 1;
                mechOidSet.elements = mechOid;
     } else
                desiredMechs = GSS_C_NULL_OID_SET;


     name_buf.value = service_name;
     name_buf.length = strlen(name_buf.value) + 1;
     maj_stat = gss_import_name(&min_stat, &name_buf,
                (gss_OID) GSS_C_NT_HOSTBASED_SERVICE, &server_name);
     if (maj_stat != GSS_S_COMPLETE) {
          display_status("importing name", maj_stat, min_stat);
          if (mechOid != GSS_C_NO_OID)
                gss_release_oid(&min_stat, &mechOid);
          return -1;
     }

     maj_stat = gss_acquire_cred(&min_stat, server_name, 0,
                                 desiredMechs, GSS_C_ACCEPT,
                                 server_creds, NULL, NULL);

     if (maj_stat != GSS_S_COMPLETE) {
          display_status("acquiring credentials", maj_stat, min_stat);
          return -1;
     }

     (void) gss_release_name(&min_stat, &server_name);

     return 0;
}


static int read_all(int fildes, char *buf, unsigned int nbyte)
{
     int ret;
     char *ptr;

     for (ptr = buf; nbyte; ptr += ret, nbyte -= ret) {
          ret = read(fildes, ptr, nbyte);
          if (ret < 0) {
               if (errno == EINTR)
                    continue;
               return(ret);
          } else if (ret == 0) {
               return(ptr-buf);
          }
     }

     return(ptr-buf);
}

/*
 * Function: recv_token
 *
 * Purpose: Reads a token from a file descriptor.
 *
 * Arguments:
 *
 *      s               (r) an open file descriptor
 *      tok             (w) the read token
 *
 * Returns: 0 on success, -1 on failure
 *
 * Effects:
 *
 * recv_token reads the token length (as a network long), allocates
 * memory to hold the data, and then reads the token data from the
 * file descriptor s.  It blocks to read the length and data, if
 * necessary.  On a successful return, the token should be freed with
 * gss_release_buffer.  It returns 0 on success, and -1 if an error
 * occurs or if it could not read all the data.
 */
int recv_token(s, tok)
     int s;
     gss_buffer_t tok;
{
     int ret, len;

     ret = read_all(s, (char *) &len, sizeof(int));
     if (ret < 0) {
          perror("reading token length");
          return -1;
     } else if (ret != 4) {
         if (display_file)
             fprintf(display_file,
                     "reading token length: %d of %d bytes read\n",
                     ret, 4);
         return -1;
     }

     tok->length = ntohl(len);
     tok->value = (char *) malloc(tok->length);
     if (tok->value == NULL) {
         if (display_file)
             fprintf(display_file,
                     "Out of memory allocating token data\n");
          return -1;
     }

     ret = read_all(s, (char *) tok->value, (OM_uint32)tok->length);
     if (ret < 0) {
          perror("reading token data");
          free(tok->value);
          return -1;
     } else if (ret != tok->length) {
          fprintf(stderr, "sending token data: %d of %d bytes written\n",
                  ret, tok->length);
          free(tok->value);
          return -1;
     }

     return 0;
}
#endif

} // namespace security
