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

/*
 * Encode a client request for the given transport version.
 *
 * Unless the message type T is explicitly exempt from adl<> support, type T
 * must be supported by both adl<> and serde encoding frameworks. When the type
 * is not exempt from adl<> support, serde is used when the version >= v2.
 *
 * The returned version indicates what level of encoding is used. This is always
 * equal to the input version, except for serde-only messags which return v2.
 * Callers are expected to further validate the runtime implications of this.
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
         * serde. this exception is present so that work on the protocol can
         * proceed in parallel with conversion of message types to use serde.
         *
         * Tracked here as part of v22.2.x release requirements:
         * https://github.com/redpanda-data/redpanda/issues/4934
         */
        return reflection::async_adl<T>{}.to(out, std::move(msg)).then([] {
            return transport_version::v0;
        });
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
        return reflection::async_adl<T>{}.from(*raw).finally(
          [p = std::move(p)] {});
    });
}

} // namespace rpc
