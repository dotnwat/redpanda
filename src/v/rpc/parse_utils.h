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

#pragma once

#include "compression/stream_zstd.h"
#include "hashing/xx.h"
#include "likely.h"
#include "reflection/async_adl.h"
#include "rpc/logger.h"
#include "rpc/types.h"
#include "seastarx.h"
#include "serde/serde.h"
#include "vlog.h"

#include <seastar/core/do_with.hh>
#include <seastar/core/future.hh>
#include <seastar/core/iostream.hh>

#include <fmt/format.h>

#include <memory>
#include <optional>

namespace rpc {
namespace detail {
inline void check_out_of_range(size_t got, size_t expected) {
    if (unlikely(got != expected)) {
        throw std::out_of_range(fmt::format(
          "parse_utils out of range. got:{} bytes and expected:{} bytes",
          got,
          expected));
    }
}
} // namespace detail

inline ss::future<std::optional<header>>
parse_header(ss::input_stream<char>& in) {
    return read_iobuf_exactly(in, size_of_rpc_header).then([](iobuf b) {
        if (b.size_bytes() != size_of_rpc_header) {
            return ss::make_ready_future<std::optional<header>>();
        }
        iobuf_parser parser(std::move(b));
        auto h = reflection::adl<header>{}.from(parser);
        if (auto got = checksum_header_only(h);
            unlikely(h.header_checksum != got)) {
            vlog(
              rpclog.info,
              "rpc header missmatching checksums. expected:{}, got:{} - {}",
              h.header_checksum,
              got,
              h);
            return ss::make_ready_future<std::optional<header>>();
        }
        return ss::make_ready_future<std::optional<header>>(h);
    });
}

inline void validate_payload_and_header(const iobuf& io, const header& h) {
    detail::check_out_of_range(io.size_bytes(), h.payload_size);
    auto in = iobuf::iterator_consumer(io.cbegin(), io.cend());
    incremental_xxhash64 hasher;
    size_t consumed = in.consume(
      io.size_bytes(), [&hasher](const char* src, size_t sz) {
          hasher.update(src, sz);
          return ss::stop_iteration::no;
      });
    detail::check_out_of_range(consumed, h.payload_size);
    const auto got_checksum = hasher.digest();
    if (h.payload_checksum != got_checksum) {
        throw std::runtime_error(fmt::format(
          "invalid rpc checksum. got:{}, expected:{}",
          got_checksum,
          h.payload_checksum));
    }
}

/*
 * the transition from adl to serde encoding in rpc requires a period of time
 * where both encodings are supported for all message types. however, we do not
 * want to extend this requirement to brand new messages / services. we enlist
 * the help of the type system to enforce these rules, but it means we need a
 * way to opt-out of adl support on a case-by-case basis for new message types.
 *
 * the `rpc_adl_exempt` type trait helper can be used to opt-out a type T from
 * adl support. a type is marked exempt by defining the type T::rpc_adl_exempt.
 * the typedef may be defined as any type such as std::{void_t, true_type}.
 *
 * Example:
 *
 *     struct exempt_msg {
 *         using rpc_adl_exempt = std::true_type;
 *         ...
 *     };
 */
template<typename, typename = void>
struct has_rpc_adl_exempt : std::false_type {};
template<typename T>
struct has_rpc_adl_exempt<T, std::void_t<typename T::rpc_adl_exempt>>
  : std::true_type {};
template<typename T>
inline constexpr auto const is_rpc_adl_exempt = has_rpc_adl_exempt<T>::value;

// ticket references
// deprecating adl in a new namespace for use here
// remove the allowance for non-serde support during transition period
// async serde
// better serde move/constT& handling

/*
 * Encode a client request or server response for the given transport version.
 *
 * Unless the message type T is explicitly exempt from adl<> support, type T
 * must be supported by both adl<> and serde encoding frameworks. When the type
 * is not exempt from adl<> support, serde is used when the version >= v2.
 *
 * Caller should log things like: version1 but serde-only message.
 *
 *
 * - server responses vs request version
 *
 *     - v2: happy path
 *     - v1: hard assertion failure. this indicates a protocol violation. the
 *     requesting peer should have made the same decision about the message
 *     being serde-only and sent the message as v2.
 *     - v0: same as v1, but weirder
 *
 *     NOTE: it may seem that the server should merely drop the connection when
 *     handling a protocol violation rather than hard-stop. however, the
 *     assumption here is that these same rules are applied when decoding the
 *     client request being responded to. and when decoding the request,
 *     handling these cases drops the connection rather than triggering a
 *     hard-stop scenario.
 *
 *
 * v0 - this will be the case for tests: we'll forcefully make a transport
 * behave like a v0 transport.
 *
 *   serde-only
 *     serde, v2: this case is asserted false. doesn't make sense
 *
 *   serde+adl
 *     adl, v0
 *   adl-only
 *     adl, v0
 *
 * v1
 *   serde-only
 *     serde, v2: ok--this would used in conjunction with feature barrier
 *     TODO: on receipt, transprot should also upgrade here.
 *     TODO: if not v2 received back then shutdown the connection
 *
 *   serde+adl
 *     adl, v1
 *
 *   adl-only
 *     adl, v0: this is the temporary hack situation.
 *
 * v2
 *   serde-only
 *     serde, v2
 *   serde+adl
 *     serde, v2
 *   adl-only
 *     adl, v0
 *
 * v3
 *   ??? need to worry?
 *
 * serde-only message handling
 * ===========================
 * - v2: happy path
 * - v1: allowed under the assumption that the message will be sent as a v2
 *   message. this is important for the case that a new connection that
 *   hasn't been upgraded via normal negotation handles a message sent
 *   shortly after a feature barrier test passes allowing serde-only
 *   messages.
 * - v0: a hard assertion failure is thrown because it indicates (1) it
 *   implies a testing environment and (2) serde messsages should never be
 *   encoded on a v0 channel.
 */
template<typename T>
ss::future<transport_version>
encode_with_version(iobuf& out, T msg, transport_version version) {
    if constexpr (serde::supported<T>) {
        if constexpr (is_rpc_adl_exempt<T>) {
            return ss::do_with(std::move(msg), [&out](const T& msg) {
                return serde::write_async(out, msg).then(
                  [] { return transport_version::v2; });
            });
        } else {
            if (version < transport_version::v2) {
                return reflection::async_adl<T>{}
                  .to(out, std::move(msg))
                  .then([version] { return version; });
            } else {
                return ss::do_with(
                  std::move(msg), [&out, version](const T& msg) {
                      return serde::write_async(out, msg).then(
                        [version] { return version; });
                  });
            }
        }
    } else {
        /*
         * TODO: remove this else block as soon as all types are supported by
         * serde. this is allowed temporarily to not serialize core RPC changes
         * with all of the conversions. until this is removed message types
         * without serde support will be encoded as v0 messages.
         *
         * Tracked here as part of v22.2.x release requirements:
         * https://github.com/redpanda-data/redpanda/issues/4934
         */
        return reflection::async_adl<T>{}.to(out, std::move(msg)).then([] {
            return transport_version::v0;
        });
    }
}

/*
 * v0 - this will be the case for tests: we'll forcefully make a transport
 * behave like a v0 transport.
 *
 *   serde-only
 TODO *     serde, vXXX: throw an exception and close the connection. doesn't
 make
 *     sense for adl only (which arrives as v0) becuase we assume that we are
 *     not doing a mixed version situation
 *
 *   serde+adl
 *     adl, v0
 *   adl-only
 *     adl, v0
 *
 * v1
 *   serde-only
 *     serde, vXXX: throw here. clients should also be sending serde at v2,
 *     including serde-only types.
 *
 *   serde+adl
 *     adl, v1
 *
 *   adl-only
 *     adl, v0: assert FALSE. prevent by design and only temp testing
 *
 * v2
 *   serde-only
 *     serde, v2
 *   serde+adl
 *     serde, v2
 *   adl-only
 *     adl, v0: assert FALSE. prevent by design and only temp testing
 *
 * v3
 *   ??? need to worry?
 */
template<typename T>
ss::future<T>
decode_with_version(iobuf_parser& parser, transport_version version) {
    if constexpr (serde::supported<T>) {
        if constexpr (is_rpc_adl_exempt<T>) {
            if (version < transport_version::v2) {
                return ss::make_exception_future<T>(
                  std::runtime_error(fmt::format(
                    "Unexpected serde-only message on v{} channel", version)));
            }
            return serde::read_async<T>(parser);
        } else {
            if (version < transport_version::v2) {
                return reflection::async_adl<T>{}.from(parser);
            } else {
                return serde::read_async<T>(parser);
            }
        }
    } else {
        /*
         * TODO: remove this else block as soon as all types are supported by
         * serde. this is allowed temporarily to avoid having to add serde
         * support for all types prior to merging core changes.
         *
         * The assertion that an adl-only type is at v0 is safe under the
         * assumption that mixed versions are not yet being tested other than in
         * explicitly supported tests.
         *
         * Tracked here as part of v22.2.x release requirements:
         * https://github.com/redpanda-data/redpanda/issues/4934
         */
        vassert(
          version == transport_version::v0,
          "Received adl-only message with version v{}",
          version);
        return reflection::async_adl<T>{}.from(parser);
    }
}

template<typename T>
ss::future<T> parse_type(ss::input_stream<char>& in, const header& h) {
    return read_iobuf_exactly(in, h.payload_size).then([h](iobuf io) {
        validate_payload_and_header(io, h);

        switch (h.compression) {
        case compression_type::none:
            break;

        case compression_type::zstd: {
            compression::stream_zstd fn;
            io = fn.uncompress(std::move(io));
            break;
        }

        default:
            return ss::make_exception_future<T>(std::runtime_error(
              fmt::format("no compression supported. header: {}", h)));
        }

        auto p = std::make_unique<iobuf_parser>(std::move(io));
        auto raw = p.get();
        return decode_with_version<T>(*raw, h.version)
          .finally([p = std::move(p)] {});
    });
}

} // namespace rpc
