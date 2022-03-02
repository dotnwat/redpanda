/*
 * Copyright 2022 Vectorized, Inc.
 *
 * Use of this software is governed by the Business Source License
 * included in the file licenses/BSL.md
 *
 * As of the Change Date specified in that file, in accordance with
 * the Business Source License, use of this software will be governed
 * by the Apache License, Version 2.0
 */
#pragma once
#include "cluster/fwd.h"
#include "seastarx.h"

#include <seastar/core/abort_source.hh>
#include <seastar/core/coroutine.hh>
#include <seastar/core/sharded.hh>
#include <seastar/util/log.hh>

/*
 *
 */
class drain_manager {
public:
    struct drain_status {};

    drain_manager(
      ss::logger&,
      cluster::controller*,
      ss::sharded<cluster::partition_manager>&);

    ss::future<> stop() { co_return; }

    /*
     * Start draining this broker.
     */
    ss::future<> drain();

    /*
     * Restore broker to a non-drain[ing] state.
     */
    ss::future<> restore();

    ss::future<drain_status> status();

private:
    ss::future<> do_drain();
    ss::future<> drain_leadership(cluster::partition_manager&);
    ss::future<> do_restore();

    ss::logger& _log;
    [[maybe_unused]] cluster::controller* _controller;
    [[maybe_unused]] ss::sharded<cluster::partition_manager>&
      _partition_manager;
    std::optional<ss::future<>> _drain;
    ss::abort_source _abort;
};
