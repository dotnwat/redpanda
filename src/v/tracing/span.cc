// Copyright 2024 Redpanda Data, Inc.
//
// Use of this software is governed by the Business Source License
// included in the file licenses/BSL.md
//
// As of the Change Date specified in that file, in accordance with
// the Business Source License, use of this software will be governed
// by the Apache License, Version 2.0

#include "tracing/span.h"

#include <utility>

namespace tracing {

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
    _buffer->submit(completed_span{
      .trace = _trace_id,
      .id = _span_id,
      .parent = _parent_span_id,
      .name = std::move(_name),
      .kind = _kind,
      .start_ns = _start_ns,
      .end_ns = now_ns(),
      .is_error = _is_error,
      .error_message = std::move(_error_message),
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
  , _error_message(std::move(o._error_message)) {}

span& span::operator=(span&& o) noexcept {
    if (this != &o) {
        // Submit current span if enabled before overwriting.
        if (_buffer) {
            _buffer->submit(completed_span{
              .trace = _trace_id,
              .id = _span_id,
              .parent = _parent_span_id,
              .name = std::move(_name),
              .kind = _kind,
              .start_ns = _start_ns,
              .end_ns = now_ns(),
              .is_error = _is_error,
              .error_message = std::move(_error_message),
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

uint64_t span::now_ns() {
    return static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch())
        .count());
}

} // namespace tracing
