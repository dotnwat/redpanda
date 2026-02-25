// Copyright 2024 Redpanda Data, Inc.
//
// Use of this software is governed by the Business Source License
// included in the file licenses/BSL.md
//
// As of the Change Date specified in that file, in accordance with
// the Business Source License, use of this software will be governed
// by the Apache License, Version 2.0

#pragma once

#include "random/generators.h"
#include "utils/uuid.h"

#include <array>
#include <cstdint>
#include <cstring>

namespace tracing {

using trace_id = std::array<uint8_t, 16>;
using span_id = std::array<uint8_t, 8>;

/// Propagated context for distributed tracing.
struct trace_context {
    trace_id trace{};
    span_id parent_span{};
    uint8_t flags{0};

    bool is_sampled() const { return (flags & 0x01) != 0; }
};

enum class span_kind : uint8_t {
    internal = 1,
    server = 2,
    client = 3,
    producer = 4,
    consumer = 5,
};

/// Generate a random 16-byte trace ID from uuid_t::create().
inline trace_id make_trace_id() {
    auto uuid = uuid_t::create();
    trace_id id{};
    std::memcpy(id.data(), &uuid.uuid(), 16);
    return id;
}

/// Generate a random 8-byte span ID.
inline span_id make_span_id() {
    auto& rng = random_generators::global();
    span_id id{};
    auto v = rng.get_int<uint64_t>();
    std::memcpy(id.data(), &v, 8);
    return id;
}

inline bool is_valid(const trace_id& id) {
    for (auto b : id) {
        if (b != 0) {
            return true;
        }
    }
    return false;
}

inline bool is_valid(const span_id& id) {
    for (auto b : id) {
        if (b != 0) {
            return true;
        }
    }
    return false;
}

} // namespace tracing
