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
  , _log_storage_target_size(std::move(log_storage_target_size))
  , _policy(_pm, _storage) {
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

/*
 * Attempts to set retention offset for cloud enabled topics such that the
 * target amount of bytes will be recovered when garbage collection is applied.
 *
 * This is greedy approach which will recover as much as possible from each
 * partition before moving on to the next.
 */
#if 0
static ss::future<size_t>
set_partition_retention_offsets(cluster::partition_manager& pm, size_t target) {
    // build a lightweight copy to avoid invalidations during iteration
    fragmented_vector<ss::lw_shared_ptr<cluster::partition>> partitions;
    for (const auto& p : pm.partitions()) {
        if (!p.second->remote_partition()) {
            continue;
        }
        partitions.push_back(p.second);
    }

    vlog(
      rlog.info,
      "Attempting to recover {} from {} remote partitions on core {}",
      human::bytes(target),
      partitions.size(),
      ss::this_shard_id());

    size_t partitions_total = 0;
    for (const auto& p : partitions) {
        if (partitions_total >= target) {
            break;
        }

        auto log = dynamic_cast<storage::disk_log_impl*>(p->log().get_impl());
        // make sure housekeeping doesn't delete the log from below us
        auto gate = log->gate().hold();

        auto segments = log->cloud_gc_eligible_segments();

        vlog(
          rlog.info,
          "Remote partition {} reports {} reclaimable segments",
          p->ntp(),
          segments.size());

        if (segments.empty()) {
            continue;
        }

        size_t log_total = 0;
        auto offset = segments.front()->offsets().committed_offset;
        for (const auto& seg : segments) {
            auto usage = co_await seg->persistent_size();
            log_total += usage.total();
            offset = seg->offsets().committed_offset;
            vlog(
              rlog.info,
              "Collecting segment {}:{}-{} estimated to recover {}",
              p->ntp(),
              seg->offsets().base_offset(),
              seg->offsets().committed_offset(),
              human::bytes(usage.total()));
            if (log_total >= target) {
                break;
            }
        }

        vlog(
          rlog.info,
          "Setting retention offset override {} estimated reclaim of {} for "
          "cloud topic {}. Total reclaim {} of target {}.",
          offset,
          log_total,
          p->ntp(),
          partitions_total,
          target);

        log->set_cloud_gc_offset(offset);
        partitions_total += log_total;
    }

    co_return partitions_total;
}
#endif

void eviction_policy::schedule::seek(size_t cursor) {
    vassert(sched_size > 0, "Seek cannot be called on an empty schedule");
    cursor = cursor % sched_size;

    // reset iterator
    shard_idx = 0;
    partition_idx = 0;

    for (; shard_idx < shards.size(); ++shard_idx) {
        const auto count = shards[shard_idx].partitions.size();
        if (cursor < count) {
            partition_idx = cursor;
            return;
        }
        cursor -= count;
    }

    vassert(false, "Seek could not find cursor location");
}

void eviction_policy::schedule::next() {
    ++partition_idx;
    while (true) {
        if (partition_idx >= shards[shard_idx].partitions.size()) {
            shard_idx = (shard_idx + 1) % shards.size();
            partition_idx = 0;
        } else {
            break;
        }
    }
}

eviction_policy::partition* eviction_policy::schedule::current() {
    return &shards[shard_idx].partitions[partition_idx];
}

ss::future<eviction_policy::schedule> eviction_policy::create_new_schedule() {
    auto shards = co_await _pm->map([this](auto& /* ignored */) {
        /*
         * the partition_manager reference is ignored since
         * collect_reclaimable_offsets already has access to a
         * sharded<partition_manager>.
         */
        return collect_reclaimable_offsets().then([](auto partitions) {
            return shard_partitions{
              .shard = ss::this_shard_id(),
              .partitions = std::move(partitions),
            };
        });
    });

    // compute the size of the schedule
    const auto size = std::reduce(
      shards.cbegin(),
      shards.cend(),
      size_t{0},
      [](size_t acc, const shard_partitions& shard) {
          return acc + shard.partitions.size();
      });

    vlog(rlog.debug, "Created new eviction schedule with {} partitions", size);
    co_return schedule(std::move(shards), size);
}

ss::future<fragmented_vector<eviction_policy::partition>>
eviction_policy::collect_reclaimable_offsets() {
    /*
     * build a lightweight copy to avoid invalidations during iteration
     */
    fragmented_vector<ss::lw_shared_ptr<cluster::partition>> partitions;
    for (const auto& p : _pm->local().partitions()) {
        if (!p.second->remote_partition()) {
            continue;
        }
        partitions.push_back(p.second);
    }

    /*
     * retention settings mirror settings found in housekeeping()
     */
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

    /*
     * in smallish batches partitions are queried for their reclaimable
     * segments. all of this information is bundled up and returned.
     */
    fragmented_vector<partition> res;
    co_await ss::max_concurrent_for_each(
      partitions.begin(), partitions.end(), 20, [&res, cfg](const auto& p) {
          auto log = dynamic_cast<storage::disk_log_impl*>(p->log().get_impl());
          auto gate = log->gate().hold(); // protect against log deletes
          return log->get_reclaimable_offsets(cfg)
            .then([&res, p](auto offsets) {
                res.push_back({
                  .group = p->group(),
                  .offsets = std::move(offsets),
                });
            })
            .finally([g = std::move(gate)] {});
      });

    /*
     * sorting by raft::group_id would provide a more stable round-robin
     * evaluation of partitions in later processing of the result, but on small
     * timescales we don't expect enough churn to make a difference, nor with a
     * large number of partitions on the system would the non-determinism
     * in the parallelism above in max_concurrent_for_each be problematic.
     */

    vlog(rlog.trace, "Reporting reclaim offsets for {} partitions", res.size());
    co_return res;
}

ss::future<> eviction_policy::install_schedule(schedule schedule) {
    /*
     * install the schedule on each core in parallel
     */
    auto total = co_await ss::map_reduce(
      schedule.shards,
      [this](auto& shard) { return install_schedule(std::move(shard)); },
      size_t(0),
      std::plus<>());

    vlog(rlog.info, "Requested truncation from {} cloud partitions", total);
}

// the per-shard counterpart of install_schedule(schedule)
ss::future<size_t> eviction_policy::install_schedule(shard_partitions shard) {
    return _pm->invoke_on(shard.shard, [shard = std::move(shard)](auto& pm) {
        size_t decisions = 0;
        for (const auto& group : shard.partitions) {
            if (!group.decision.has_value()) {
                continue;
            }

            auto p = pm.partition_for(group.group);
            if (!p) {
                continue;
            }

            auto log = dynamic_cast<storage::disk_log_impl*>(
              p->log().get_impl());
            log->set_cloud_gc_offset(group.decision.value());
            ++decisions;

            vlog(
              rlog.trace,
              "Setting retention offset override {} estimated reclaim "
              "of "
              "{} for cloud topic {} / group {}",
              group.decision.value(),
              human::bytes(group.total),
              p->ntp(),
              group.group);
        }
        return decisions;
    });
}

size_t eviction_policy::evict_balanced_from_level(
  schedule& sched, size_t target_excess, const level_selector& selector) {
    /*
     * round robin reclaim oldest segment until we've met the target size or we
     * exhaust the amount of available space to reclaim in this phase.
     */
    bool progress = false;
    size_t total = 0;
    auto partition = sched.current();
    const auto first = partition;

    while (true) {
        /*
         * if it's the first time here in this phase, initialize iter
         */
        auto level = selector(partition);
        if (partition->level != level) {
            partition->level = level;
            partition->iter = level->begin();
            vlog(
              rlog.info,
              "Initializing group {} phase 2 iterator with {} candidates",
              partition->group,
              level->size());
        }

        if (partition->iter.value() == level->end()) {
            vlog(
              rlog.info,
              "Reached the end of phase 2 candidates for group {}",
              partition->group);
        } else {
            total += partition->iter.value()->size;
            partition->total += partition->iter.value()->size;
            partition->decision = partition->iter.value()->offset;
            progress = true;
            vlog(
              rlog.info,
              "Group {}: remove offset {} size {} total {} overall total {}",
              partition->group,
              partition->decision,
              partition->iter.value()->size,
              partition->total,
              total);
            ++partition->iter.value();
        }

        ++_cursor;
        sched.next();
        partition = sched.current();
        if (partition == first) {
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

size_t eviction_policy::evict_until_local_retention(
  schedule& sched, size_t target_excess) {
    return evict_balanced_from_level(sched, target_excess, [](auto group) {
        return &group->offsets.effective_local_retention;
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
        auto schedule = co_await _policy.create_new_schedule();
        vlog(rlog.info, "Schedule size {}", schedule.sched_size);
        if (schedule.sched_size > 0) {
            schedule.seek(_policy.cursor());
            /*
             *
             */
            auto estimate = _policy.evict_until_local_retention(
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
        co_await _policy.install_schedule(std::move(schedule));
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
