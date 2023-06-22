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
 *  - restart run loop if an exception occurs
 *  - on small disk sizes, the low space threshold is smaller than min size /
 *  blocking threshold
 *  - should trimming be disabled on config change in cache service?
 *  - if we notice that we are below the must-have should we increase it?
 *  - add buffers so we avoid thrashing. like when we reset cache size back to
 *  max.
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

    /*
     * the space manager currently only controls the cache disk, which is not
     * runtime configurable. so if the cache isn't enabled, there is not point
     * in starting the control loop.
     */
    if (_cache == nullptr) {
        vlog(
          rlog.info,
          "Cloud cache is not enabled. Disk space manager will have no effect");
        co_return;
    }

    if (ss::this_shard_id() == run_loop_core) {
        ssx::spawn_with_gate(_gate, [this] { return run_loop(); });
        _cache_disk_nid = _storage_node->local().register_disk_notification(
          node::disk_type::cache, [this](node::disk_space_info info) {
              _cache_disk_info = info;
              maybe_signal_run_loop();
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

void disk_space_manager::maybe_signal_run_loop() {
    /*
     * in low free space or degraded state wake up the control loop immediately
     * to deal with the situation.
     */
    if (_cache_disk_info.alert != disk_space_alert::ok) {
        _control_sem.signal();
        return;
    }

    /*
     * no active disk disk space alert is positive, but if we had placed
     * restrictions on the cache that are no longer necessarily we should lift
     * those to allow its capacity to increase towards its target.
     */
    if (_cache->local().max_bytes() < _cache->local().target_max_bytes()) {
        _control_sem.signal();
        return;
    }
}

ss::future<> disk_space_manager::run_loop() {
    vassert(ss::this_shard_id() == run_loop_core, "Run on wrong core");

    /*
     * we want the code here to actually run a little, but the final shape of
     * configuration options is not yet known.
     */
    constexpr auto frequency = std::chrono::seconds(3);
    constexpr auto max_bytes_step_size = 100_MiB;

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
            /*
             * restore the target max size of the cache?
             */
            if (
              _cache->local().max_bytes()
              < _cache->local().target_max_bytes()) {
                vlog(
                  rlog.info,
                  "cache disk health ok resetting max size to target {} from "
                  "{}",
                  human::bytes(_cache->local().target_max_bytes()),
                  human::bytes(_cache->local().max_bytes()));
                _cache->local().set_max_bytes_override();
            }
            continue;
        }

        vlog(
          rlog.info,
          "cache disk capacity {} free {} thresholds low {} degraded {}",
          human::bytes(_cache_disk_info.total),
          human::bytes(_cache_disk_info.free),
          human::bytes(_cache_disk_info.low_space_threshold),
          human::bytes(_cache_disk_info.degraded_threshold));

        auto cache_targets
          = co_await _pm->local().get_cloud_cache_disk_usage_target();

        vlog(
          rlog.info,
          "cache size {} effective max {} target {} capacity wanted {} "
          "required {}",
          human::bytes(_cache->local().current_size()),
          human::bytes(_cache->local().max_bytes()),
          human::bytes(_cache->local().target_max_bytes()),
          human::bytes(cache_targets.target_bytes),
          human::bytes(cache_targets.target_min_bytes));

        /*
         * the first knob we can adjust in a low free space state.
         *
         * the cache may not contain enough data available for removal that
         * will take the system out of the alert state, but we can still play
         * nice by not allowing the cache to use any additional free space.
         *
         * the max_bytes_tirm_threshold value is the effective maximum size of
         * cache that achives this. this is like a "shrink_to_fit" operation.
         */
        const auto cache_max_trim_threshold
          = _cache->local().max_bytes_trim_threshold();
        vlog(
          rlog.info,
          "cache effective max size set to trim threshold {}",
          human::bytes(cache_max_trim_threshold));
        _cache->local().set_max_bytes_override(cache_max_trim_threshold);

        // amount of additional free space needed to clear the alert
        const auto free_space_needed = _cache_disk_info.low_space_threshold
                                       - _cache_disk_info.free;

        // the size by which the cache may be reduced without violating the
        // current must-have space requirement.
        const auto max_reduction = _cache->local().max_bytes()
                                   - std::min(
                                     cache_targets.target_min_bytes,
                                     _cache->local().max_bytes());

        vlog(
          rlog.info,
          "cache disk needs {} freed {} available in cache service",
          human::bytes(free_space_needed),
          human::bytes(max_reduction));

        /*
         * the second knob to adjust.
         *
         * reduce the amount of data in the cache. the reduction is applied
         * incrementally to both smooth out the behavior as well as avoid nuking
         * a ton of data if the reduction is due to a temporary blip.
         */
        if (max_reduction > 0) {
            const auto step = std::min(max_reduction, max_bytes_step_size);
            const auto new_max_bytes = _cache->local().max_bytes() - step;
            vlog(
              rlog.info,
              "cache disk max size reduced by {} to {}",
              human::bytes(step),
              human::bytes(new_max_bytes));
            _cache->local().set_max_bytes_override(new_max_bytes);
            co_await _cache->local().trim();
        }

        // TODO add info/warn for threshold violations
    }
}

} // namespace storage
