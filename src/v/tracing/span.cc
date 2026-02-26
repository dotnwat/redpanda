// Copyright 2024 Redpanda Data, Inc.
//
// Use of this software is governed by the Business Source License
// included in the file licenses/BSL.md
//
// As of the Change Date specified in that file, in accordance with
// the Business Source License, use of this software will be governed
// by the Apache License, Version 2.0

#include "tracing/span.h"

#include "config/configuration.h"
#include "random/generators.h"

#include <utility>

namespace tracing {

span::span(ss::sstring name, span_kind kind) {
    auto* buf = current_span_buffer();
    if (!buf || !config::shard_local_cfg().tracing_enabled()) {
        return;
    }
    auto rate = config::shard_local_cfg().tracing_sample_rate();
    if (rate < 1.0) {
        auto& rng = random_generators::global();
        auto v = rng.get_int<uint32_t>(0, 999);
        if (v >= static_cast<uint32_t>(rate * 1000.0)) {
            return;
        }
    }
    _buffer = buf;
    _trace_id = make_trace_id();
    _span_id = make_span_id();
    _name = std::move(name);
    _kind = kind;
    _start_ns = now_ns();
}

span::span(ss::sstring name, span_kind kind, const trace_context& parent) {
    if (!parent.is_sampled()) {
        return;
    }
    auto* buf = current_span_buffer();
    if (!buf) {
        return;
    }
    _buffer = buf;
    _trace_id = parent.trace;
    _span_id = make_span_id();
    _parent_span_id = parent.parent_span;
    _name = std::move(name);
    _kind = kind;
    _start_ns = now_ns();
}

span::span(span_buffer& buf, ss::sstring name, span_kind kind)
  : _buffer(&buf)
  , _trace_id(make_trace_id())
  , _span_id(make_span_id())
  , _parent_span_id{}
  , _name(std::move(name))
  , _kind(kind)
  , _start_ns(now_ns()) {}

span::span(
  span_buffer& buf,
  ss::sstring name,
  span_kind kind,
  const trace_context& parent)
  : _buffer(&buf)
  , _trace_id(parent.trace)
  , _span_id(make_span_id())
  , _parent_span_id(parent.parent_span)
  , _name(std::move(name))
  , _kind(kind)
  , _start_ns(now_ns()) {}

span::~span() {
    if (!_buffer) {
        return;
    }
    _buffer->submit(
      completed_span{
        .trace = _trace_id,
        .id = _span_id,
        .parent = _parent_span_id,
        .name = std::move(_name),
        .kind = _kind,
        .start_ns = _start_ns,
        .end_ns = now_ns(),
        .is_error = _is_error,
        .error_message = std::move(_error_message),
        .attributes = std::move(_attributes),
      });
}

span::span(span&& o) noexcept
  : _buffer(std::exchange(o._buffer, nullptr))
  , _trace_id(o._trace_id)
  , _span_id(o._span_id)
  , _parent_span_id(o._parent_span_id)
  , _name(std::move(o._name))
  , _kind(o._kind)
  , _start_ns(o._start_ns)
  , _is_error(o._is_error)
  , _error_message(std::move(o._error_message))
  , _attributes(std::move(o._attributes)) {}

span& span::operator=(span&& o) noexcept {
    if (this != &o) {
        // Submit current span if enabled before overwriting.
        if (_buffer) {
            _buffer->submit(
              completed_span{
                .trace = _trace_id,
                .id = _span_id,
                .parent = _parent_span_id,
                .name = std::move(_name),
                .kind = _kind,
                .start_ns = _start_ns,
                .end_ns = now_ns(),
                .is_error = _is_error,
                .error_message = std::move(_error_message),
                .attributes = std::move(_attributes),
              });
        }
        _buffer = std::exchange(o._buffer, nullptr);
        _trace_id = o._trace_id;
        _span_id = o._span_id;
        _parent_span_id = o._parent_span_id;
        _name = std::move(o._name);
        _kind = o._kind;
        _start_ns = o._start_ns;
        _is_error = o._is_error;
        _error_message = std::move(o._error_message);
        _attributes = std::move(o._attributes);
    }
    return *this;
}

trace_context span::context() const {
    return trace_context{
      .trace = _trace_id,
      .parent_span = _span_id,
      .flags = static_cast<uint8_t>(_buffer ? 0x01 : 0x00),
    };
}

void span::set_error(ss::sstring message) {
    if (!_buffer) {
        return;
    }
    _is_error = true;
    _error_message = std::move(message);
}

void span::set_attribute(ss::sstring key, ss::sstring value) {
    if (!_buffer) {
        return;
    }
    _attributes.emplace_back(std::move(key), std::move(value));
}

void span::set_attribute(ss::sstring key, int64_t value) {
    if (!_buffer) {
        return;
    }
    _attributes.emplace_back(std::move(key), ss::to_sstring(value));
}

uint64_t span::now_ns() {
    return static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch())
        .count());
}

} // namespace tracing
