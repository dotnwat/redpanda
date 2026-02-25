// Copyright 2024 Redpanda Data, Inc.
//
// Use of this software is governed by the Business Source License
// included in the file licenses/BSL.md
//
// As of the Change Date specified in that file, in accordance with
// the Business Source License, use of this software will be governed
// by the Apache License, Version 2.0

#include "tracing/span_buffer.h"

#include "base/vlog.h"

#include <seastar/core/coroutine.hh>
#include <seastar/util/log.hh>

ss::logger tlog("tracing");

namespace tracing {

void span_buffer::submit(completed_span span) {
    if (_buffer.size() >= max_buffer_size) {
        if (!_overflow_logged) {
            vlog(
              tlog.warn,
              "Trace span buffer full (max {}), dropping spans",
              max_buffer_size);
            _overflow_logged = true;
        }
        return;
    }
    _overflow_logged = false;
    _buffer.push_back(std::move(span));
}

chunked_vector<completed_span> span_buffer::drain() {
    _overflow_logged = false;
    return std::exchange(_buffer, {});
}

ss::future<> span_buffer::start() { co_return; }

ss::future<> span_buffer::stop() {
    _buffer.clear();
    co_return;
}

} // namespace tracing
