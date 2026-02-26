// Copyright 2024 Redpanda Data, Inc.
//
// Use of this software is governed by the Business Source License
// included in the file licenses/BSL.md
//
// As of the Change Date specified in that file, in accordance with
// the Business Source License, use of this software will be governed
// by the Apache License, Version 2.0

#pragma once

#include "container/chunked_vector.h"
#include "tracing/trace_context.h"

#include <seastar/core/sharded.hh>
#include <seastar/core/sstring.hh>

#include <cstdint>
#include <utility>
#include <vector>

namespace tracing {

/// A string key-value attribute on a span.
using span_attribute = std::pair<ss::sstring, ss::sstring>;

/// A completed span ready for export.
struct completed_span {
    trace_id trace{};
    span_id id{};
    span_id parent{};
    ss::sstring name;
    span_kind kind{span_kind::internal};
    uint64_t start_ns{0};
    uint64_t end_ns{0};
    bool is_error{false};
    ss::sstring error_message;
    std::vector<span_attribute> attributes;
};

/// Per-shard buffer that collects completed spans for export.
/// Only the owning shard calls submit/drain — no locking needed.
///
/// On start(), each shard registers itself as the thread-local span
/// buffer so that spans can be created anywhere without explicit
/// plumbing — similar to how Seastar loggers are globally accessible.
class span_buffer : public ss::peering_sharded_service<span_buffer> {
public:
    static constexpr size_t max_buffer_size = 4096;

    void submit(completed_span span);
    chunked_vector<completed_span> drain();

    ss::future<> start();
    ss::future<> stop();

private:
    chunked_vector<completed_span> _buffer;
    bool _overflow_logged{false};
};

/// Get the current shard's span_buffer, or nullptr if tracing is not
/// initialized. Safe to call from any shard at any time.
span_buffer* current_span_buffer();

} // namespace tracing
