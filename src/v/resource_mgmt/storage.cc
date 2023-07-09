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
#include "storage/disk_log_impl.h"
#include "utils/human.h"
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

    while (!_gate.is_closed()) {
        try {
            if (_enabled()) {
                co_await _control_sem.wait(
                  config::shard_local_cfg().log_storage_max_usage_interval(),
                  std::max(_control_sem.current(), size_t(1)));
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

void disk_space_manager::eviction_schedule::seek(size_t cursor) {
    vassert(sched_size > 0, "");
    cursor = cursor % sched_size;

    // reset iterator
    shard_idx = 0;
    group_idx = 0;

    for (; shard_idx < offsets.size(); ++shard_idx) {
        const auto count = offsets[shard_idx].offsets.size();
        if (cursor < count) {
            group_idx = cursor;
            return;
        }
        cursor -= count;
    }

    vassert(false, "");
}

void disk_space_manager::eviction_schedule::next() {
    ++group_idx;
    while (true) {
        if (group_idx >= offsets[shard_idx].offsets.size()) {
            shard_idx = (shard_idx + 1) % offsets.size();
            group_idx = 0;
        } else {
            break;
        }
    }
}

disk_space_manager::group_offsets*
disk_space_manager::eviction_schedule::current() {
    return &offsets[shard_idx].offsets[group_idx];
}

/*
 * Collect reclaimable partition offsets on the local core.
 */
ss::future<fragmented_vector<disk_space_manager::group_offsets>>
disk_space_manager::collect_reclaimable_offsets() {
    // build a lightweight copy to avoid invalidations during iteration
    fragmented_vector<ss::lw_shared_ptr<cluster::partition>> partitions;
    for (const auto& p : _pm->local().partitions()) {
        if (!p.second->remote_partition()) {
            continue;
        }
        partitions.push_back(p.second);
    }

    // retention settings mirror settings found in housekeeping()
    const auto collection_threshold = [this] {
        const auto& lm = _storage->local().log_mgr();
        if (!lm.config().delete_retention().has_value()) {
            return model::timestamp(0);
        }
        const auto now = model::timestamp::now().value();
        const auto retention = lm.config().delete_retention().value().count();
        return model::timestamp(now - retention);
    };

    gc_config cfg(
      collection_threshold(),
      _storage->local().log_mgr().config().retention_bytes());

    // TODO run in parallel
    fragmented_vector<group_offsets> res;
    for (const auto& p : partitions) {
        auto log = dynamic_cast<storage::disk_log_impl*>(p->log().get_impl());
        auto gate = log->gate().hold(); // protect against log deletes
        auto offsets = co_await log->get_reclaimable_offsets(cfg);
        res.push_back({
          .group = p->group(),
          .offsets = std::move(offsets),
        });
    }

    // sorting by raft::group_id would provide a more stable round-robin
    // evaluation of partitions in later processing of the result, but on small
    // timescales we don't expect enough churn to make a difference.

    co_return res;
}

/*
 * Collect reclaimable partition offsets from each core.
 *
 * Notice that the partition manager drives cross-core execution via
 * sharded::map but we ignore its reference parameter and re-index into the
 * local sharded partition manager (and other services) in the helper function.
 */
ss::future<disk_space_manager::eviction_schedule>
disk_space_manager::initialize_eviction_schedule() {
    auto offsets = co_await _pm->map([this](auto& /* ignored */) {
        return collect_reclaimable_offsets().then([](auto offsets) {
            return shard_offsets{
              .shard = ss::this_shard_id(),
              .offsets = std::move(offsets),
            };
        });
    });

    const auto size = std::reduce(
      offsets.cbegin(),
      offsets.cend(),
      size_t{0},
      [](size_t acc, const shard_offsets& offsets) {
          return acc + offsets.offsets.size();
      });

    co_return eviction_schedule(std::move(offsets), size);
}

size_t disk_space_manager::apply_phase2_local_retention(
  eviction_schedule& sched, size_t target_excess) {
    /*
     * round robin reclaim oldest segment until we've met the target size or we
     * exhaust the amount of available space to reclaim in this phase.
     */
    bool progress = false;
    size_t total = 0;
    auto group = sched.current();
    const auto begin = group;

    while (true) {
        /*
         * if it's the first time here in this phase, initialize iter
         */
        if (group->phase != &group->offsets.effective_local_retention) {
            group->phase = &group->offsets.effective_local_retention;
            group->iter = group->offsets.effective_local_retention.begin();
            vlog(
              rlog.info,
              "Initializing group {} phase 2 iterator with {} candidates",
              group->group,
              group->offsets.effective_local_retention.size());
        }

        if (
          group->iter.value()
          == group->offsets.effective_local_retention.end()) {
            vlog(
              rlog.info,
              "Reached the end of phase 2 candidates for group {}",
              group->group);
        } else {
            /*
             * TODO grop.iter->size needs to be incremental for this to work
             */
            total += group->iter.value()->size;
            group->total += group->iter.value()->size;
            group->decision = group->iter.value()->offset;
            progress = true;
            vlog(
              rlog.info,
              "Group {}: remove offset {} size {} total {} overall total {}",
              group->group,
              group->decision,
              group->iter.value()->size,
              group->total,
              total);
            ++group->iter.value();
        }

        ++_cursor;
        sched.next();
        group = sched.current();
        if (group == begin) {
            if (!progress) {
                vlog(
                  rlog.info,
                  "Phase 2 did not find any more candidate segments");
                break;
            }
            vlog(rlog.info, "Phase 2 examined all partitions. Starting over.");
            progress = false;
        }

        if (total > target_excess) {
            vlog(
              rlog.info,
              "Phase 2 completing after finding enough data to reclaim");
            break;
        }
    }

    return total;
}

ss::future<>
disk_space_manager::broadcast_schedule(eviction_schedule schedule) {
    co_await ss::parallel_for_each(schedule.offsets, [this](auto& sched) {
        return _pm->invoke_on(sched.shard, [&sched](auto& pm) {
            for (const auto& group : sched.offsets) {
                if (!group.decision.has_value()) {
                    continue;
                }
                auto p = pm.partition_for(group.group);
                if (!p) {
                    continue;
                }
                auto log = dynamic_cast<storage::disk_log_impl*>(
                  p->log().get_impl());
                vlog(
                  rlog.info,
                  "Setting {}/{} gc offset {}",
                  group.group,
                  p->ntp(),
                  group.decision.value());
                log->set_cloud_gc_offset(group.decision.value());
            }
        });
    });
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
        vlog(
          rlog.info,
          "Log storage usage {} <= target size {}. No work to do.",
          human::bytes(usage.usage.total()),
          human::bytes(target_size));
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
     * target, then a second knob must be turned: removing data that would
     * otherwise not be removed by normal garbage collection, such as data that
     * has bene uploaded to cloud storage but falls below local retention.
     */
    if (target_excess > usage.reclaim.retention) {
        vlog(
          rlog.info,
          "XXXXXXXXXXXXXXXX Log storage usage {} > target size {} by {}. "
          "Garbage collection "
          "expected to recover {}. Overriding tiered storage retention to "
          "recover {}. Total estimated available to recover {}",
          human::bytes(usage.usage.total()),
          human::bytes(target_size),
          human::bytes(target_excess),
          human::bytes(usage.reclaim.retention),
          human::bytes(target_excess - usage.reclaim.retention),
          human::bytes(usage.reclaim.available));

        /*
         * In the overall eviction policy phase 1 corresponds to a prioritized
         * application of normal GC. When that is insufficient we reach this
         * point which starts with phase 2 of the policy.
         */
        auto schedule = co_await initialize_eviction_schedule();
        vlog(rlog.info, "Schedule size {}", schedule.sched_size);
        if (schedule.sched_size > 0) {
            schedule.seek(_cursor);
            /*
             *
             */
            auto estimate = apply_phase2_local_retention(
              schedule, target_excess);
            vlog(
              stlog.info,
              "Phase 2 total estimate reclaim {} target {}",
              human::bytes(estimate),
              human::bytes(target_excess));
        }

        /*
         *
         */
        co_await broadcast_schedule(std::move(schedule));
    } else {
        vlog(
          rlog.info,
          "XXXXXXXXXXXXX-OK Log storage usage {} > target size {} by {}. "
          "Garbage collection "
          "expected to recover {}.",
          human::bytes(usage.usage.total()),
          human::bytes(target_size),
          human::bytes(target_excess),
          human::bytes(usage.reclaim.retention));
    }

    /*
     * ask storage across all nodes to apply retention rules asap.
     */
    co_await _storage->invoke_on_all([](api& api) { api.trigger_gc(); });
}

} // namespace storage
