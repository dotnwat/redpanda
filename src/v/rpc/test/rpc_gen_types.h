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

#include "reflection/adl.h"
#include "rpc/parse_utils.h"
#include "seastarx.h"
#include "serde/envelope.h"
#include "serde/serde.h"

#include <seastar/core/sstring.hh>

#include <cstdint>

namespace cycling {
struct ultimate_cf_slx {
    int x = 42;
};
struct nairo_quintana {
    int x = 43;
};
struct san_francisco {
    int x = 44;
};
struct mount_tamalpais {
    int x = 45;
};
} // namespace cycling

namespace echo {
struct echo_req {
    ss::sstring str;
};

struct echo_resp {
    ss::sstring str;
};

struct cnt_req {
    uint64_t expected;
};

struct cnt_resp {
    uint64_t expected;
    uint64_t current;
};

enum class failure_type { throw_exception, exceptional_future, none };

using throw_req = failure_type;

struct throw_resp {
    ss::sstring reply;
};

/*
 * echo methods with req/resp that support encodings:
 * - adl only
 * - serde only
 * - serde and adl
 */
struct echo_req_adl_only {
    ss::sstring str;
};

struct echo_resp_adl_only {
    ss::sstring str;
};

static_assert(!serde::is_serde_compatible_v<echo_req_adl_only>);
static_assert(!serde::is_serde_compatible_v<echo_resp_adl_only>);
static_assert(!rpc::is_rpc_adl_exempt<echo_req_adl_only>);
static_assert(!rpc::is_rpc_adl_exempt<echo_resp_adl_only>);

struct echo_req_adl_serde
  : serde::envelope<echo_req_adl_serde, serde::version<1>> {
    ss::sstring str;

    void serde_write(iobuf& out) const {
        using serde::write;
        write(out, str + "_to_sas");
    }

    friend void read_nested(
      iobuf_parser& in, echo_req_adl_serde& r, const size_t bytes_left_limit) {
        const auto hdr = serde::read_header<echo_req_adl_serde>(
          in, bytes_left_limit);
        using serde::read_nested;
        ss::sstring tmp;
        read_nested(in, tmp, hdr._bytes_left_limit);
        r.str = tmp + "_from_sas";
    }
};

struct echo_resp_adl_serde
  : serde::envelope<echo_resp_adl_serde, serde::version<1>> {
    ss::sstring str;

    void serde_write(iobuf& out) const {
        using serde::write;
        write(out, str + "_to_sas");
    }

    friend void read_nested(
      iobuf_parser& in, echo_resp_adl_serde& r, const size_t bytes_left_limit) {
        const auto hdr = serde::read_header<echo_resp_adl_serde>(
          in, bytes_left_limit);
        using serde::read_nested;
        ss::sstring tmp;
        read_nested(in, tmp, hdr._bytes_left_limit);
        r.str = tmp + "_from_sas";
    }
};

static_assert(serde::is_serde_compatible_v<echo_req_adl_serde>);
static_assert(serde::is_serde_compatible_v<echo_resp_adl_serde>);
static_assert(!rpc::is_rpc_adl_exempt<echo_req_adl_serde>);
static_assert(!rpc::is_rpc_adl_exempt<echo_resp_adl_serde>);

struct echo_req_serde_only
  : serde::envelope<echo_req_serde_only, serde::version<1>> {
    using rpc_adl_exempt = std::true_type;
    ss::sstring str;

    void serde_write(iobuf& out) const {
        using serde::write;
        write(out, str + "_to_sso");
    }

    friend void read_nested(
      iobuf_parser& in, echo_req_serde_only& r, const size_t bytes_left_limit) {
        const auto hdr = serde::read_header<echo_req_serde_only>(
          in, bytes_left_limit);
        using serde::read_nested;
        ss::sstring tmp;
        read_nested(in, tmp, hdr._bytes_left_limit);
        r.str = tmp + "_from_sso";
    }
};

struct echo_resp_serde_only
  : serde::envelope<echo_resp_serde_only, serde::version<1>> {
    using rpc_adl_exempt = std::true_type;
    ss::sstring str;

    void serde_write(iobuf& out) const {
        using serde::write;
        write(out, str + "_to_sso");
    }

    friend void read_nested(
      iobuf_parser& in,
      echo_resp_serde_only& r,
      const size_t bytes_left_limit) {
        const auto hdr = serde::read_header<echo_resp_serde_only>(
          in, bytes_left_limit);
        using serde::read_nested;
        ss::sstring tmp;
        read_nested(in, tmp, hdr._bytes_left_limit);
        r.str = tmp + "_from_sso";
    }
};

static_assert(serde::is_serde_compatible_v<echo_req_serde_only>);
static_assert(serde::is_serde_compatible_v<echo_resp_serde_only>);
static_assert(rpc::is_rpc_adl_exempt<echo_req_serde_only>);
static_assert(rpc::is_rpc_adl_exempt<echo_resp_serde_only>);

} // namespace echo

namespace reflection {
template<>
struct adl<echo::echo_req_adl_only> {
    void to(iobuf& out, echo::echo_req_adl_only&& r) {
        reflection::serialize(out, r.str + "_to_aao");
    }
    echo::echo_req_adl_only from(iobuf_parser& in) {
        return echo::echo_req_adl_only{
          .str = adl<ss::sstring>{}.from(in) + "_from_aao",
        };
    }
};

template<>
struct adl<echo::echo_resp_adl_only> {
    void to(iobuf& out, echo::echo_resp_adl_only&& r) {
        reflection::serialize(out, r.str + "_to_aao");
    }
    echo::echo_resp_adl_only from(iobuf_parser& in) {
        return echo::echo_resp_adl_only{
          .str = adl<ss::sstring>{}.from(in) + "_from_aao",
        };
    }
};

template<>
struct adl<echo::echo_req_adl_serde> {
    void to(iobuf& out, echo::echo_req_adl_serde&& r) {
        reflection::serialize(out, r.str + "_to_aas");
    }
    echo::echo_req_adl_serde from(iobuf_parser& in) {
        return echo::echo_req_adl_serde{
          .str = adl<ss::sstring>{}.from(in) + "_from_aas",
        };
    }
};

template<>
struct adl<echo::echo_resp_adl_serde> {
    void to(iobuf& out, echo::echo_resp_adl_serde&& r) {
        reflection::serialize(out, r.str + "_to_aas");
    }
    echo::echo_resp_adl_serde from(iobuf_parser& in) {
        return echo::echo_resp_adl_serde{
          .str = adl<ss::sstring>{}.from(in) + "_from_aas",
        };
    }
};
} // namespace reflection
