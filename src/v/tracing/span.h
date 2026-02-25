// Copyright 2024 Redpanda Data, Inc.
//
// Use of this software is governed by the Business Source License
// included in the file licenses/BSL.md
//
// As of the Change Date specified in that file, in accordance with
// the Business Source License, use of this software will be governed
// by the Apache License, Version 2.0

#pragma once

#include "tracing/span_buffer.h"
#include "tracing/trace_context.h"

#include <seastar/core/sstring.hh>

#include <chrono>
#include <cstdint>

namespace tracing {

/// RAII span. Submits a completed_span to the span_buffer on destruction.
///
/// A default-constructed span is disabled (no-op). All operations on a
/// disabled span are safe and do nothing.
class span {
public:
    /// Disabled span (no-op). All operations are safe no-ops.
    span() = default;

    /// Root span — starts a new trace.
    span(span_buffer& buf, ss::sstring name, span_kind kind);

    /// Child span — continues an existing trace.
    span(
      span_buffer& buf,
      ss::sstring name,
      span_kind kind,
      const trace_context& parent);

    ~span();

    span(span&&) noexcept;
    span& operator=(span&&) noexcept;
    span(const span&) = delete;
    span& operator=(const span&) = delete;

    /// Get the trace context for propagation to child spans.
    trace_context context() const;

    /// Whether this span is enabled (will be exported).
    bool is_enabled() const { return _buffer != nullptr; }

    /// Mark this span as an error.
    void set_error(ss::sstring message);

private:
    static uint64_t now_ns();

    span_buffer* _buffer{nullptr};
    trace_id _trace_id{};
    span_id _span_id{};
    span_id _parent_span_id{};
    ss::sstring _name;
    span_kind _kind{span_kind::internal};
    uint64_t _start_ns{0};
    bool _is_error{false};
    ss::sstring _error_message;
};

} // namespace tracing
