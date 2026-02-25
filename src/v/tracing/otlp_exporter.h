// Copyright 2024 Redpanda Data, Inc.
//
// Use of this software is governed by the Business Source License
// included in the file licenses/BSL.md
//
// As of the Change Date specified in that file, in accordance with
// the Business Source License, use of this software will be governed
// by the Apache License, Version 2.0

#pragma once

#include "bytes/iobuf.h"
#include "tracing/span_buffer.h"

#include <seastar/core/abort_source.hh>
#include <seastar/core/gate.hh>
#include <seastar/core/sharded.hh>
#include <seastar/core/timer.hh>

namespace tracing {

/// Periodically drains spans from all shards and exports them to an
/// OpenTelemetry Collector via HTTP (OTLP/protobuf).
/// Runs on shard 0 only, following the metrics_reporter pattern.
class otlp_exporter {
public:
    static constexpr ss::shard_id shard = 0;

    explicit otlp_exporter(ss::sharded<span_buffer>&);

    ss::future<> start();
    ss::future<> stop();

private:
    void arm_timer();
    ss::future<> do_export();
    ss::future<> send(iobuf payload);

    ss::sharded<span_buffer>& _spans;
    ss::timer<> _timer;
    ss::gate _gate;
    ss::abort_source _as;
};

} // namespace tracing
