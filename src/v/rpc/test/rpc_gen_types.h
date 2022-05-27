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
struct echo_req : serde::envelope<echo_req, serde::version<1>> {
    ss::sstring str;
};

struct echo_resp : serde::envelope<echo_resp, serde::version<1>> {
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

} // namespace echo

namespace reflection {
template<>
struct adl<echo::echo_req> {
    void to(iobuf& out, echo::echo_req&& r) {
        reflection::serialize(out, r.str);
    }
    echo::echo_req from(iobuf_parser& in) {
        return echo::echo_req{
          .str = adl<ss::sstring>{}.from(in),
        };
    }
};
template<>
struct adl<echo::echo_resp> {
    void to(iobuf& out, echo::echo_resp&& r) {
        reflection::serialize(out, r.str);
    }
    echo::echo_resp from(iobuf_parser& in) {
        return echo::echo_resp{
          .str = adl<ss::sstring>{}.from(in),
        };
    }
};
} // namespace reflection
