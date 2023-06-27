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
#include "vlog.h"

#include <seastar/util/log.hh>

static ss::logger rlog("resource_mgmt");

namespace storage {

disk_space_manager::disk_space_manager(
  config::binding<bool> enabled,
  config::binding<std::optional<uint64_t>> log_storage_target_size,
  ss::sharded<storage::api>* storage,
  ss::sharded<storage::node>* storage_node,
  ss::sharded<cloud_storage::cache>* cache,
  ss::sharded<cluster::partition_manager>* pm)
  : _enabled(std::move(enabled))
  , _storage(storage)
  , _storage_node(storage_node)
  , _cache(cache->local_is_initialized() ? cache : nullptr)
  , _pm(pm)
  , _log_storage_target_size(std::move(log_storage_target_size)) {
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
          node::disk_type::cache,
          [this](node::disk_space_info info) { _cache_disk_info = info; });
        _data_disk_nid = _storage_node->local().register_disk_notification(
          node::disk_type::data,
          [this](node::disk_space_info info) { _data_disk_info = info; });
    }
    co_return;
}

ss::future<> disk_space_manager::stop() {
    vlog(rlog.info, "Stopping disk space manager service");
    if (ss::this_shard_id() == run_loop_core) {
        _storage_node->local().unregister_disk_notification(
          node::disk_type::cache, _cache_disk_nid);
        _storage_node->local().unregister_disk_notification(
          node::disk_type::data, _data_disk_nid);
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
    constexpr auto frequency = std::chrono::seconds(30);

    while (!_gate.is_closed()) {
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

        if (_log_storage_target_size().has_value()) {
            co_await manage_data_disk(_log_storage_target_size().value());
        }
    }
}

ss::future<> disk_space_manager::manage_data_disk(uint64_t target_size) {
    /*
     * query log storage usage across all cores
     */
    storage::usage_report usage;
    try {
        usage = co_await _storage->local().disk_usage();
    } catch (...) {
        vlog(
          rlog.info,
          "Unable to collect log storage usage. Skipping management tick: "
          "{}",
          std::current_exception());
        co_return;
    }

    /*
     * how much data from log storage do we need to remove from disk to be able
     * to stay below the current target size?
     */
    const auto target_excess = usage.usage.total() < target_size
                                 ? 0
                                 : usage.usage.total() - target_size;
    if (target_excess <= 0) {
        co_return;
    }

    /*
     * when log storage has exceeded the target usage, then there are some knobs
     * that can be adjusted to help stay below this target.
     *
     * the first knob is to prioritize garbage collection. this is normally a
     * periodic task performed by the storage layer. if it hasn't run recently,
     * or it has been spending most of its time doing low impact compaction
     * work, then we can instead immediately begin applying retention rules.
     *
     * the effect of gc is defined entirely by topic configuration and current
     * state of storage. so in order to turn the first knob we need only trigger
     * gc across shards.
     *
     * however, if current retention is not sufficient to bring usage below the
     * target, then a second knob must be turned: overriding local retention
     * targets for cloud-enabled topics, removing data that has been backed up
     * into the cloud.
     */
    if (target_excess > usage.reclaim.retention) {
        /*
         * TODO: prior to triggering a priorty GC run across cores (below),
         * adjust local retention settings according to the to-be-created policy
         * for selecting victims.
         *
         * https://github.com/redpanda-data/core-internal/issues/268
         */
    }

    /*
     * ask storage across all nodes to apply retention rules asap.
     */
    co_await _storage->invoke_on_all([](api& api) { api.trigger_gc(); });
}

} // namespace storage
