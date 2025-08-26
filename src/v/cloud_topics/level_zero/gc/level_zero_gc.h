/*
 * Copyright 2025 Redpanda Data, Inc.
 *
 * Licensed as a Redpanda Enterprise file under the Redpanda Community
 * License (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 *
 * https://github.com/redpanda-data/redpanda/blob/master/licenses/rcl.md
 */
#pragma once

#include "cloud_io/remote.h"
#include "cloud_topics/types.h"
#include "model/fundamental.h"

#include <seastar/core/condition-variable.hh>
#include <seastar/core/future.hh>

#include <expected>

namespace cloud_io {
class remote;
}

namespace cloud_topics {

/*
 * Garbage collection for level-zero data objects.
 *
 * Every L0 data object is associated with a global epoch:
 *
 *    .../00000-<uuid>
 *    .../00999-<uuid>
 *    .../00999-<uuid>
 *    .../01005-<uuid>
 *
 * The process of L0 garbage collection involves first determining an epoch
 * value for which it is safe to delete all L0 objects tagged with epochs less
 * than order equal to the safe epoch, and then requesting the underlying
 * storage system to delete these qualifying objects.
 *
 *
 * A node with any non-zero ingress rate will upload at least four L0
 * objects per second. Thus a five node cluster will upload a minimum of
 * about 20 objects per second. In contrast, a cluster with an ingress
 * rate of 4 GB/s using 4 MB L0 data object will upload around 1000 objects
 * per second.
 *
 * AWS S3 allows batch deletes of 1000 objects per request. So as we
 * approach supporting 4 GB/s in a cluster L0 GC will need to be able
 * perform around one maximum batch delete request per second. It remains to
 * be seen how much load this will place on a single core, and we should
 * therefore be prepared to scale out L0 GC as needed, either to more cores
 * or more nodes.
 *
 * check lexiographic ordering of output
 *
 * most object storage systems state explicitly that object listings are
 * sorted in lexiographic order. however, some lesser used systems
 * either (1) explicitly state that this is not the case or (2) have
 * configuration options that allow lexiographic ordering to be
 * disabled.
 *
 * currently cloud topics assumes that object listings are in
 * lexiographic ordering to simplify the implementation through the use
 * of a stateless GC process. when used with a system that produces
 * non-lexiographic orderings, the stateless process will operate
 * correctly, but may be highly inefficient.
 *
 * next we check the output and watch for non-lexiographic orderings so
 * that we can warn appropriately. if such a warning is encountered,
 * then it may be an indication that cloud topics should adopt a more
 * flexible approach to tracking cleaned epochs.
 */
class level_zero_gc {
    // GC grace period. TODO should be configuration option
    static constexpr std::chrono::seconds min_gc_grace_period{5};

public:
    /*
     * Object storage interface used by L0 GC.
     */
    class object_storage {
    public:
        object_storage() = default;
        object_storage(const object_storage&) = delete;
        object_storage(object_storage&&) = delete;
        object_storage& operator=(const object_storage&) = delete;
        object_storage& operator=(object_storage&&) = delete;
        virtual ~object_storage() = default;

        /*
         * Implementations are expected to limit the listing to only L0 data
         * objects, and provide the listing in _globally_ lexiographic order.
         */
        virtual seastar::future<std::expected<
          cloud_storage_clients::client::list_bucket_result,
          cloud_storage_clients::error_outcome>>
        list_objects(seastar::abort_source*) = 0;

        virtual seastar::future<std::expected<void, cloud_io::upload_result>>
        delete_objects(
          seastar::abort_source*,
          std::vector<cloud_storage_clients::client::list_bucket_item>)
          = 0;
    };

    /*
     * Interface for computing the maximum epoch eligible for GC.
     */
    class epoch_source {
    public:
        epoch_source() = default;
        epoch_source(const epoch_source&) = default;
        epoch_source(epoch_source&&) = delete;
        epoch_source& operator=(const epoch_source&) = default;
        epoch_source& operator=(epoch_source&&) = delete;
        virtual ~epoch_source() = default;

        /*
         * L0 objects with epochs <= the return value may be deleted. An
         * expected return value of std::nullopt indicates that no GC eligible
         * epoch could yet be determined.
         */
        virtual seastar::future<
          std::expected<std::optional<cluster_epoch>, std::string>>
        max_gc_eligible_epoch(seastar::abort_source*) = 0;
    };

public:
    /*
     * Construct with the given storage and epoch providers. This interface is
     * intended to be used by tests which swap in mock implementations.
     */
    level_zero_gc(
      std::unique_ptr<object_storage>, std::unique_ptr<epoch_source>);

    /*
     * Construct with default implementations of storage and epoch providers.
     */
    level_zero_gc(cloud_io::remote*, cloud_storage_clients::bucket_name);

    /*
     * Request that GC be started or stopped. These can be called multiple times
     * and in any order. The last invocation will eventually take effect.
     */
    void start();
    void stop();

    /*
     * Request and wait for GC to be completely stopped. After calling shutdown,
     * calling start() or stop() will have no effect.
     */
    seastar::future<> shutdown();

private:
    std::unique_ptr<object_storage> storage_;
    std::unique_ptr<epoch_source> epoch_source_;

    bool should_run_;
    bool should_shutdown_;
    seastar::abort_source asrc_;
    seastar::condition_variable worker_cv_;
    seastar::future<> worker_;

    seastar::future<> worker();
    enum class collection_error : int8_t;
    seastar::future<std::expected<size_t, collection_error>> try_to_collect();
};

} // namespace cloud_topics
