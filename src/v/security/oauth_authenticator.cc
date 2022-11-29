#include "security/oauth_authenticator.h"

#include "security/logger.h"
#include "utils/utf8.h"

#include <boost/algorithm/string.hpp>
#include <re2/re2.h>
#include <re2/stringpiece.h>

#define KEY "[A-Za-z]+"
#define VALUE "[\\x21-\\x7E \t\r\n]+"
#define SEPARATOR "\u0001"
#define SASL_NAME "(?:[\\x01-\\x7F&&[^=,]]|=2C|=3D)+"
#define KVPAIRS "(?:" KEY "=" VALUE SEPARATOR ")*"

#define CLIENT_FIRST_MESSAGE_RE                                                \
    "n,(?:a=(" SASL_NAME "))?," SEPARATOR "(" KVPAIRS ")" SEPARATOR

#define AUTH_PATTERN "([\\w]+)[ ]+([-_\\.a-zA-Z0-9]+)"

namespace security {

result<bytes> oauth_authenticator::authenticate(bytes_view auth_bytes) {
    // if (response.length == 1 && response[0] ==
    // OAuthBearerSaslClient.BYTE_CONTROL_A && errorMessage != null) {
    vlog(seclog.info, "XXXXXXX {}", auth_bytes.size());

    auto view = std::string_view(
      reinterpret_cast<const char*>(auth_bytes.data()), auth_bytes.size());
    validate_utf8(view);

    static thread_local const re2::RE2 re(CLIENT_FIRST_MESSAGE_RE);
    vassert(re.ok(), "client-first-message regex failure: {}", re.error());

    re2::StringPiece authzid;
    re2::StringPiece kvpairs;

    if (!re2::RE2::FullMatch(view, re, &authzid, &kvpairs)) {
        vlog(seclog.info, "XXXXXXX---- NOT MATCH {}", view);
        return bytes{};
    }

    absl::flat_hash_map<ss::sstring, ss::sstring> extensions;

    // split on "," following the "," prefix
    // TODO: share this with scram_algorithm same thing duplicated
    std::vector<std::string> extension_pairs;
    boost::split(extension_pairs, kvpairs, boost::is_any_of(SEPARATOR));

    // split pairs on first "=". the value part may also contain "="
    for (const auto& pair : extension_pairs) {
        if (pair.empty()) { // trailing separator
            continue;
        }
        auto it = std::find(pair.cbegin(), pair.cend(), '=');
        if (unlikely(it == pair.cend())) {
            throw std::runtime_error(fmt_with_ctx(
              ssx::sformat, "Invalid oauth client first message: {}", view));
        }
        extensions.emplace(
          ss::sstring(pair.cbegin(), it), ss::sstring(it + 1, pair.cend()));
    }

    vlog(seclog.info, "...... authzid={}", authzid);
    for (auto e : extensions) {
        vlog(seclog.info, "...... key={} value={}", e.first, e.second);
    }

    // remove auth key and that leaves the extensions

    static thread_local const re2::RE2 auth_re(AUTH_PATTERN);
    vassert(
      auth_re.ok(),
      "auth pattern client-first-message regex failure: {}",
      auth_re.error());

    re2::StringPiece scheme;
    re2::StringPiece token;

    if (!re2::RE2::FullMatch(
          extensions["auth"].c_str(), auth_re, &scheme, &token)) {
        vlog(seclog.info, "XXXXXXX---- NOT MATCH {}", extensions["auth"]);
        return bytes{};
    }

    vlog(seclog.info, "LLLLLLL scheme {}", scheme);
    vlog(seclog.info, "LLLLLLL token {}", token);


    // NEXT
    // public class OAuthBearerValidatorCallbackHandler implements
    // AuthenticateCallbackHandler {

    return bytes{};
}

} // namespace security
