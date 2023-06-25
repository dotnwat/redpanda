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

#include "cluster/metadata_cache.h"
#include "cluster/partition_manager.h"
#include "config/configuration.h"
#include "utils/human.h"
#include "vlog.h"

#include <seastar/util/log.hh>

static ss::logger rlog("resource_mgmt");

namespace storage {

disk_space_manager::disk_space_manager(
  config::binding<bool> enabled,
  config::binding<std::optional<uint64_t>> log_storage_max_size,
  ss::sharded<storage::api>* storage,
  ss::sharded<cloud_storage::cache>* cache,
  ss::sharded<cluster::partition_manager>* pm,
  ss::sharded<cluster::metadata_cache>* mc)
  : _enabled(std::move(enabled))
  , _log_storage_max_size(std::move(log_storage_max_size))
  , _storage(storage)
  , _cache(cache->local_is_initialized() ? cache : nullptr)
  , _pm(pm)
  , _mc(mc) {
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
    ssx::spawn_with_gate(_gate, [this] { return run_loop(); });
    co_return;
}

ss::future<> disk_space_manager::stop() {
    vlog(rlog.info, "Stopping disk space manager service");
    _control_sem.broken();
    co_await _gate.close();
}

ss::future<> disk_space_manager::run_loop() {
    /*
     * we want the code here to actually run a little, but the final shape of
     * configuration options is not yet known.
     */
    constexpr auto frequency = std::chrono::seconds(3);

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
          "Log storage usage total {} - data {} index {} compaction {}",
          logs_usage.usage.total(),
          logs_usage.usage.data,
          logs_usage.usage.index,
          logs_usage.usage.compaction);

        if (
          _log_storage_max_size().has_value()
          && logs_usage.usage.total() > _log_storage_max_size().value()) {
            vlog(
              rlog.info,
              "Log storage usage {} exceeds configured max {}. Kafka produce "
              "API blocked",
              human::bytes(logs_usage.usage.total()),
              human::bytes(_log_storage_max_size().value()));
            co_await _mc->invoke_on_all(
              [](auto& mc) { mc.block_writes(true); });
        } else {
            co_await _mc->invoke_on_all(
              [](auto& mc) { mc.block_writes(false); });
        }
    }
}

} // namespace storage
