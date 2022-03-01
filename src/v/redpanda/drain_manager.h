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

    ss::future<> start_draining();
    ss::future<> stop_draining();
    ss::future<drain_status> status();

private:
    ss::logger& _log;
    [[maybe_unused]] cluster::controller* _controller;
    [[maybe_unused]] ss::sharded<cluster::partition_manager>&
      _partition_manager;
};
