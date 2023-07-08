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

#pragma once

#include "config/property.h"
#include "raft/types.h"
#include "seastarx.h"
#include "ssx/semaphore.h"
#include "storage/node.h"

#include <seastar/core/sharded.hh>

#include <iostream>

namespace cloud_storage {
class cache;
}

namespace cluster {
class partition_manager;
}

namespace storage {

class api;
class node;

/*
 *
 */
class disk_space_manager {
    static constexpr ss::shard_id run_loop_core = 0;

public:
    disk_space_manager(
      config::binding<bool> enabled,
      config::binding<std::optional<uint64_t>> log_storage_target_size,
      ss::sharded<storage::api>* storage,
      ss::sharded<storage::node>* storage_node,
      ss::sharded<cloud_storage::cache>* cache,
      ss::sharded<cluster::partition_manager>* pm);

    disk_space_manager(disk_space_manager&&) noexcept = delete;
    disk_space_manager& operator=(disk_space_manager&&) noexcept = delete;
    disk_space_manager(const disk_space_manager&) = delete;
    disk_space_manager& operator=(const disk_space_manager&) = delete;
    ~disk_space_manager() = default;

    ss::future<> start();
    ss::future<> stop();

private:
    config::binding<bool> _enabled;
    ss::sharded<storage::api>* _storage;
    ss::sharded<storage::node>* _storage_node;
    ss::sharded<cloud_storage::cache>* _cache;
    ss::sharded<cluster::partition_manager>* _pm;

    node::notification_id _cache_disk_nid;
    node::notification_id _data_disk_nid;
    // details from last disk notification
    node::disk_space_info _cache_disk_info{};
    node::disk_space_info _data_disk_info{};

    ss::future<> manage_data_disk(uint64_t target_size);
    config::binding<std::optional<uint64_t>> _log_storage_target_size;

    /*
     * eviction policy
     */
    struct group_offsets {
        raft::group_id group;
        reclaimable_offsets offsets;

        /*
         * points to the next offset to consider for eviction in the offset
         * container for the current phase of evaluation. the only reason this
         * is an optional<T> is because the seastar iterator doesn't have a
         * default constructor.
         */
        std::optional<ss::chunked_fifo<reclaimable_offsets::offset>::iterator> iter;

        /*
         * pointer to one of the offset groups in the offsets member. this
         * pointer allows policy evaluation to know when the iterator needs to
         * initialized for the given phase.
         */
        ss::chunked_fifo<reclaimable_offsets::offset>* phase{nullptr};

        /*
         *
         */
        std::optional<model::offset> decision;
    };

    struct shard_offsets {
        ss::shard_id shard;
        fragmented_vector<group_offsets> offsets;
    };

    /*
     * round robin iteration
     */
    struct eviction_schedule {
        std::vector<shard_offsets> offsets;

        size_t shard_idx{0};
        size_t group_idx{0};

        explicit eviction_schedule(std::vector<shard_offsets> offsets)
          : offsets(std::move(offsets)) {}

        // number of elements in the offsets container
        size_t size() const;

        /*
         * reposition the iterator at the cursor location.
         *
         * preconditions:
         *   - container is not empty (size() != 0)
         *   - normalize cursor with cursor % size()
         */
        void seek(size_t cursor);

        /*
         * advance the iterator
         *
         * preconditions:
         *   - seek() has been invoked
         */
        void next();

        /*
         * return current partition's reclaimable offsets
         *
         * preconditions:
         *   - seek() has been invoked
         */
        group_offsets* current();
    };

    /*
     * Collects from each shard a summary of reclaimable partition offsets.
     * Each set is tagged with its corresponding raft group id. The reason
     * for this is that the results will be analyzed by the eviction policy
     * to produce an eviction schedule. That schedule needs to be able to
     * identify partitions without tracking raw pointers, and we'd also like
     * to avoid the heavy weight model::ntp representation.
     */
    ss::future<eviction_schedule> initialize_eviction_schedule();
    ss::future<fragmented_vector<group_offsets>> collect_reclaimable_offsets();

    // cursor is used to approximate a round-robin schedule when applying
    // policy phases that select data to be evicted.
    size_t _cursor{0};

    void apply_phase2_local_retention(eviction_schedule&, size_t);

    ss::gate _gate;
    ss::future<> run_loop();
    ssx::semaphore _control_sem{0, "resource_mgmt::space_manager"};
};

} // namespace storage
