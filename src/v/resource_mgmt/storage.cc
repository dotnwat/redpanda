/*
 * Copyright 2023 Redpanda Data, Inc.
 *
 * Use of this software is governed by the Business Source License
 * included in the file licenses/BSL.md
 *
 * As of the Change Date specified in that file, in accordance with
 * the Business Source License, use of this software will be governed
 * by the Apache License, Version 2.0
 */

#include "storage.h"

#include "cloud_storage/cache_service.h"
#include "cluster/partition_manager.h"
#include "utils/human.h"
#include "vlog.h"

#include <seastar/util/log.hh>

static ss::logger rlog("resource_mgmt");

/*
 * TODO
 *  - something seems whack with the debug endpoint for changing statvfs lies
 *  - what should be the run-loop frequency when in a not-ok-state?
 *  - should we have an internal alert below the normal alert that we use to
 *  start making room so that we don't reach the alert status?
 *  - cache service target max size should be the adjusted amount calculated in
 *  trim() function
 */

namespace storage {

disk_space_manager::disk_space_manager(
  config::binding<bool> enabled,
  ss::sharded<storage::api>* storage,
  ss::sharded<storage::node>* storage_node,
  ss::sharded<cloud_storage::cache>* cache,
  ss::sharded<cluster::partition_manager>* pm)
  : _enabled(std::move(enabled))
  , _storage(storage)
  , _storage_node(storage_node)
  , _cache(cache->local_is_initialized() ? cache : nullptr)
  , _pm(pm) {
    _enabled.watch([this] {
        vlog(
          rlog.info,
          "{} disk space manager control loop",
          _enabled() ? "Enabling" : "Disabling");
        _control_sem.signal();
    });
}

ss::future<> disk_space_manager::start() {
    vlog(
      rlog.info,
      "Starting disk space manager service ({})",
      _enabled() ? "enabled" : "disabled");
    if (ss::this_shard_id() == run_loop_core) {
        ssx::spawn_with_gate(_gate, [this] { return run_loop(); });
        _cache_disk_nid = _storage_node->local().register_disk_notification(
          node::disk_type::cache, [this](node::disk_space_info info) {
              _cache_disk_info = info;
              if (_cache_disk_info.alert != disk_space_alert::ok) {
                  _control_sem.signal();
              }
          });
    }
    co_return;
}

ss::future<> disk_space_manager::stop() {
    vlog(rlog.info, "Stopping disk space manager service");
    if (ss::this_shard_id() == run_loop_core) {
        _storage_node->local().unregister_disk_notification(
          node::disk_type::cache, _cache_disk_nid);
    }
    _control_sem.broken();
    co_await _gate.close();
}

ss::future<> disk_space_manager::run_loop() {
    vassert(ss::this_shard_id() == run_loop_core, "Run on wrong core");

    /*
     * we want the code here to actually run a little, but the final shape of
     * configuration options is not yet known.
     */
    constexpr auto frequency = std::chrono::seconds(1);

    /*
     * the run loop can currently only control the cache. the cache cannot be
     * runtime enabled. so if it isn't enabled, there is no reason to keep the
     * run loop active.
     */
    if (_cache == nullptr) {
        vlog(
          rlog.info,
          "Stopping storage management control loop. Nothing to control");
        co_return;
    }

    while (true) {
        try {
            if (_enabled()) {
                co_await _control_sem.wait(
                  frequency, std::max(_control_sem.current(), size_t(1)));
            } else {
                co_await _control_sem.wait();
            }
        } catch (const ss::semaphore_timed_out&) {
            // time for some controlling
        }

        if (!_enabled()) {
            continue;
        }

        if (_cache_disk_info.alert == disk_space_alert::ok) {
            continue;
        }

        vlog(rlog.info, "XXX threshold low {} threshold degraded {}",
                human::bytes(_cache_disk_info.low_space_threshold),
                human::bytes(_cache_disk_info.degraded_threshold));

        // 1. how much space do we need to find and remove in order to get out
        // of the low disk situation? this should come from the local monitor,
        // since that is who is also providing the alerts!

        // 2. look at how much space is currently being used by the cache. it
        // may be the case that there is not enough data in the cache to purge
        // to fix the alert, but that doens't mean we shouldn't play nice:
        //
        //     1. we can still remove some data perhaps
        //     2. we don't want to make things worse
        //
        // First step will be to clamp the effective max bytes at the current
        // size of the cache so that it doesn't grow any further. This isn't
        // strictly necessary, but it avoids wasting time in slow-start by
        // jumping directly to the point where reducitions in max bytes will
        // matter.

        // 3. Start reducing max bytes. Say... 1 GB at a time until we reach
        // must have limit and then stop there. The time step should be adjusted
        // to take into account time for trimming as well as the update
        // frequency for disk stat.

        // the cache owns its own target maximum size. this might be a fixed
        // size such as 50 GB, or a percentage of total disk capacity.
        const auto target_cache_max_bytes = _cache->local().target_max_bytes();

        // 1. calculate what the effective cache size should be in order to get
        // out of the low disk situation. then let's incrementally try to reach
        // this. meaning, let's knock of 1 GB then trim. take a moment, repeat.
        // we want to be aggressive, but also smooth: what if there were a temp
        // spike in space usage or a bug / error in accounting? we wouldn't want
        // to just nuke all of the cache.

        // 2. if the effective rate drop below the nice-to-have then let's start
        // complaining. maybe info level is fine.

        // 3. if we reach the must-have level, then let's drop a warning. hey:
        // we aren't going to go any further, something isn't good with your
        // disk you'll probably run out of space and writes to teh cache will be
        // blocked.

        // 4. how do we raise the threshold back up?

        vlog(rlog.info, "ZZZ not OKOKOKOKO");

        // respond to disk alerts...
        // the key here is that if we have no alerts active, then there isn't
        // much to do (yet). maybe other policies will be more active about
        // things.
        //
        // NEXT: get an alert!

        vlog(
          rlog.info, "XXX target-cache-max-bytes {}", target_cache_max_bytes);

        /*
         * Collect cache and logs storage usage information. These accumulate
         * across all shards (despite the local() accessor). If a failure occurs
         * we wait rather than operate with a reduced set of information.
         */
        cloud_storage::cache_usage_target cache_usage_target;
        try {
            cache_usage_target
              = co_await _pm->local().get_cloud_cache_disk_usage_target();
        } catch (...) {
            vlog(
              rlog.info,
              "Unable to collect cloud cache usage: {}",
              std::current_exception());
            continue;
        }

        storage::usage_report logs_usage;
        try {
            logs_usage = co_await _storage->local().disk_usage();
        } catch (...) {
            vlog(
              rlog.info,
              "Unable to collect log storage usage: {}",
              std::current_exception());
            continue;
        }

        vlog(
          rlog.info,
          "Cloud storage cache target minimum size {} nice to have {}",
          cache_usage_target.target_min_bytes,
          cache_usage_target.target_bytes);

        vlog(
          rlog.info,
          "Log storage usage total {} - data {} index {} compaction {}",
          logs_usage.usage.total(),
          logs_usage.usage.data,
          logs_usage.usage.index,
          logs_usage.usage.compaction);

        vlog(
          rlog.info,
          "Log storage usage available for reclaim local {} total {}",
          logs_usage.reclaim.retention,
          logs_usage.reclaim.available);

        vlog(
          rlog.info,
          "Log storage usage target minimum size {} nice to have {}",
          logs_usage.target.min_capacity,
          logs_usage.target.min_capacity_wanted);
    }
}

} // namespace storage
