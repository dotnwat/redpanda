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
#include "cloud_topics/object_utils.h"

#include <seastar/core/coroutine.hh>
#include <seastar/core/sleep.hh>
#include <seastar/util/later.hh>

namespace cloud_topics {

class object_storage_remote_impl : public level_zero_gc::object_storage {
public:
    static constexpr std::chrono::seconds timeout{5};
    static constexpr std::chrono::seconds backoff{1};

    object_storage_remote_impl(
      seastar::abort_source* asrc,
      cloud_io::remote* remote,
      cloud_storage_clients::bucket_name bucket)
      : asrc_(asrc)
      , remote_(remote)
      , bucket_(std::move(bucket)) {}

    seastar::future<std::expected<
      cloud_storage_clients::client::list_bucket_result,
      cloud_storage_clients::error_outcome>>
    list_objects() override {
        retry_chain_node rtc(*asrc_, timeout, backoff);
        auto res = co_await remote_->list_objects(
          bucket_, rtc, object_path_factory::level_zero_data_dir());
        if (res.has_value()) {
            co_return res.assume_value();
        }
        co_return std::unexpected(res.assume_error());
    }

    seastar::future<std::expected<void, cloud_io::upload_result>>
    delete_objects(
      std::vector<cloud_storage_clients::client::list_bucket_item> objects)
      override {
        retry_chain_node rtc(*asrc_, timeout, backoff);
        auto keys
          = objects | std::views::transform([](auto& obj) { return obj.key; })
            | std::ranges::to<std::vector<cloud_storage_clients::object_key>>();
        auto res = co_await remote_->delete_objects(
          bucket_, keys, rtc, [](auto) {});
        if (res == cloud_io::upload_result::success) {
            co_return std::expected<void, cloud_io::upload_result>();
        }
        co_return std::unexpected(res);
    }

private:
    seastar::abort_source* asrc_;
    cloud_io::remote* remote_;
    const cloud_storage_clients::bucket_name bucket_;
};

class epoch_source_cluster_impl : public level_zero_gc::epoch_source {
public:
    seastar::future<std::expected<std::optional<cluster_epoch>, std::string>>
    max_gc_eligible_epoch() override {
        /*
         * There is more work to do before we can fully integrate here. In the
         * mean time do not allow any L0 data objects to be collected.
         */
        co_return std::nullopt;
    }
};

level_zero_gc::level_zero_gc(
  std::unique_ptr<object_storage> storage,
  std::unique_ptr<epoch_source> epoch_source)
  : storage_(std::move(storage))
  , epoch_source_(std::move(epoch_source))
  , worker_sem_(0, "level_zero_gc/worker")
  , worker_(worker())
  , last_gc_(seastar::lowres_clock::now() - min_period) {}

level_zero_gc::level_zero_gc(
  cloud_io::remote* remote, cloud_storage_clients::bucket_name bucket)
  : level_zero_gc(
      std::make_unique<object_storage_remote_impl>(
        &asrc_, remote, std::move(bucket)),
      std::make_unique<epoch_source_cluster_impl>()) {}

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
    if (!res.has_value()) {
        vlog(
          cd_log.debug,
          "Received error listing objects during L0 GC: {}",
          res.error());
        co_return;
    }

    const auto maybe_max_gc_epoch
      = co_await epoch_source_->max_gc_eligible_epoch();
    if (!maybe_max_gc_epoch.has_value()) {
        vlog(
          cd_log.debug,
          "Received error retrieving GC eligible epoch: {}",
          maybe_max_gc_epoch.error());
        co_return;
    }

    const auto max_gc_epoch = maybe_max_gc_epoch.value();
    if (!max_gc_epoch.has_value()) {
        vlog(cd_log.debug, "No GC eligible epoch currently exists");
        co_return;
    }

    const auto max_gc_birthday = std::chrono::system_clock::now()
                                 - min_gc_grace_period;

    // objects that can be safely deleted
    std::vector<cloud_storage_clients::client::list_bucket_item>
      gc_eligible_objects;

    // used to detect unsorted object listings
    seastar::sstring last_key;
    std::optional<cluster_epoch> last_epoch;

    for (auto& object : res.value().contents) {
        auto res = object_path_factory::level_zero_path_to_epoch(object.key);

        // validate expected L0 object name format, and extract epoch
        if (!res.has_value()) {
            vlog(
              cd_log.error,
              "Unable to parse epoch during L0 GC: {}",
              res.error());
            co_return;
        }

        // check that output is ordered by epoch. not fatal. see class comment.
        if (!last_epoch.has_value()) {
            last_key = object.key;
            last_epoch = res.value();
        }
        if (res.value() < last_epoch) {
            constexpr std::chrono::minutes rate_limit{1};
            static seastar::logger::rate_limit rate(rate_limit);
            vloglr(
              cd_log,
              seastar::log_level::warn,
              rate,
              "Non-lexiographic object listing detected during L0 GC {} < {}",
              object.key,
              last_key);
        }
        last_key = object.key;
        last_epoch = res.value();

        // object's epoch is not yet eligible for collection
        if (res.value() > max_gc_epoch.value()) {
            continue;
        }

        // object is too young
        if (object.last_modified < max_gc_birthday) {
            continue;
        }

        gc_eligible_objects.push_back(object);
    }

    co_await storage_->delete_objects(std::move(gc_eligible_objects));
}

} // namespace cloud_topics
