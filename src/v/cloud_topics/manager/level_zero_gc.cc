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
#include "cloud_topics/logger.h"

#include <seastar/core/coroutine.hh>
#include <seastar/core/sleep.hh>
#include <seastar/util/later.hh>

namespace cloud_topics {

class object_storage_remote_impl : public level_zero_gc::object_storage {
public:
    object_storage_remote_impl(
      cloud_io::remote* remote, cloud_storage_clients::bucket_name bucket)
      : remote_(remote)
      , bucket_(std::move(bucket))
      , prefix_("cluster_metadaasdfta") {}

    // TODO see all list_objects params
    seastar::future<cloud_io::list_result> list_objects() override {
        seastar::abort_source asrc;
        retry_chain_node rtc(
          asrc, std::chrono::seconds(5), std::chrono::seconds(1));
        auto res = co_await remote_->list_objects(bucket_, rtc, prefix_);

        // check prefix
        // check ordering

        co_return res;
    }

private:
    cloud_io::remote* remote_;
    const cloud_storage_clients::bucket_name bucket_;
    const cloud_storage_clients::object_key prefix_;
};

level_zero_gc::level_zero_gc(std::unique_ptr<object_storage> storage)
  : storage_(std::move(storage))
  , worker_sem_(0, "level_zero_gc/worker")
  , worker_(worker())
  , last_gc_(seastar::lowres_clock::now() - min_period) {}

level_zero_gc::level_zero_gc(
  cloud_io::remote* remote, cloud_storage_clients::bucket_name bucket)
  : level_zero_gc(
      std::make_unique<object_storage_remote_impl>(remote, std::move(bucket))) {
}

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

        // this can be abortable on shutdown signal. for normal stop signal
        // does it matter if it is sleeping? nah, just check for run flag
        // after waking up.
        co_await seastar::sleep(std::chrono::seconds(min_period));

        co_await gc();
    }
    vlog(cd_log.info, "XXX GC worker exiting");
}

seastar::future<> level_zero_gc::gc() {
    const auto res = co_await storage_->list_objects();
    if (res.has_error()) {
        vlog(cd_log.info, "XXX error listing {}", res.error());
        co_return;
    }

    const auto& objects = res.value().contents;
    vlog(cd_log.info, "XXX: num obj {}", objects.size());
    for (auto& object : objects) {
        vlog(cd_log.info, "XXX: see object {}", object.key);
    }
}

} // namespace cloud_topics
