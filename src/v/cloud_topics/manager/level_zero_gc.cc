/*
 * Copyright 2025 Redpanda Data, Inc.
 *
 * Licensed as a Redpanda Enterprise file under the Redpanda Community
 * License (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 *
 * https://github.com/redpanda-data/redpanda/blob/master/licenses/rcl.md
 */
#include "cloud_topics/manager/level_zero_gc.h"

#include "base/vlog.h"
#include "cloud_io/remote.h"
#include "cloud_topics/logger.h"

#include <seastar/core/coroutine.hh>
#include <seastar/core/sleep.hh>
#include <seastar/util/later.hh>

namespace cloud_topics {

level_zero_gc::level_zero_gc(
  cloud_io::remote* remote, cloud_storage_clients::bucket_name bucket)
  : remote_(remote)
  , bucket_(std::move(bucket))
  , worker_sem_(0, "level_zero_gc/worker")
  , worker_(worker())
  , last_gc_(seastar::lowres_clock::now() - min_period) {}

void level_zero_gc::start() {
    vlog(cd_log.info, "XXX Starting cloud topics L0 GC worker");
    should_run_ = true;
    worker_cv_.signal();
}

void level_zero_gc::stop() {
    vlog(cd_log.info, "XXX Stopping cloud topics L0 GC worker");
    should_run_ = false;
}

// need extra flag to indicate that worker should exit
seastar::future<> level_zero_gc::shutdown() {
    should_exit_ = true;
    stop();
    worker_cv_.signal();
    // should have try/ignore eception around this
    co_await std::exchange(worker_, seastar::now());
}

// wrap with retry/restart loop
seastar::future<> level_zero_gc::worker() {
    while (!should_exit_) {
        co_await worker_cv_.wait(
          [this] { return should_run_ || should_exit_; });
        if (should_exit_) {
            continue;
        }

        // this can be abortable on shutdown signal. for normal stop signal does
        // it matter if it is sleeping? nah, just check for run flag after
        // waking up.
        co_await seastar::sleep(std::chrono::seconds(min_period));

        co_await gc();
    }
    vlog(cd_log.info, "XXX GC worker exiting");
}

seastar::future<> level_zero_gc::gc() {
    vlog(cd_log.info, "XXX: running GC :)");
    co_return;
}

} // namespace cloud_topics
