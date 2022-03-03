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

#include <chrono>

/*
 *
 */
class drain_manager {
    static constexpr size_t max_parallel_transfers = 25;
    static constexpr std::chrono::duration transfer_throttle
      = std::chrono::seconds(5);

public:
    struct drain_status {};

    drain_manager(
      ss::logger&,
      ss::sharded<cluster::partition_manager>&);

    ss::future<> start();
    ss::future<> stop();

    /*
     * Start draining this broker.
     */
    ss::future<> drain();

    /*
     * Restore broker to a non-drain[ing] state.
     */
    ss::future<> restore();

    /*
     * Check the status of the draining process.
     */
    std::optional<drain_status> status();

private:
    ss::future<> task();
    ss::future<> do_drain();
    ss::future<> do_restore();

    ss::logger& _log;
    ss::sharded<cluster::partition_manager>& _partition_manager;
    std::optional<ss::future<>> _drain;
    bool _draining{false};
    bool _stop{false};
    ss::semaphore _sem{0};
};
