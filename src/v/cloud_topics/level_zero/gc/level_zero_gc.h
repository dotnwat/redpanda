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
#include "ssx/semaphore.h"

#include <seastar/core/condition-variable.hh>
#include <seastar/core/future.hh>

#include <expected>

namespace cloud_io {
class remote;
}

namespace cloud_topics {

/*
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
    // Avoid thrashing (e.g. cause by leadership flapping) by requiring a
    // minimum delay between GC worker activations.
    static constexpr std::chrono::seconds min_period{5};

    // GC grace period. TODO should be configuration option
    static constexpr std::chrono::seconds min_gc_grace_period{5};

public:
    /*
     * Object storage interface used by L0 GC.
     *
     * Implementations should constrain object storage access to _only_ L0 data
     * objects. For example, it is assumed (but also verified) that calls to
     * the `list_objects` interface return only L0 data objects.
     */
    class object_storage {
    public:
        object_storage() = default;
        object_storage(const object_storage&) = delete;
        object_storage(object_storage&&) = delete;
        object_storage& operator=(const object_storage&) = delete;
        object_storage& operator=(object_storage&&) = delete;
        virtual ~object_storage() = default;

        virtual seastar::future<std::expected<
          cloud_storage_clients::client::list_bucket_result,
          cloud_storage_clients::error_outcome>>
        list_objects() = 0;

        virtual seastar::future<std::expected<void, cloud_io::upload_result>>
          delete_objects(
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

        // L0 objects with epochs <= the return value may be deleted. An
        // expected return value of std::nullopt indicates that no GC eligible
        // epoch could yet be determined.
        virtual seastar::future<
          std::expected<std::optional<cluster_epoch>, std::string>>
        max_gc_eligible_epoch() = 0;
    };

    // Construct using the given storage provider
    explicit level_zero_gc(
      std::unique_ptr<object_storage>, std::unique_ptr<epoch_source>);

    // Construct using the default storage provider
    level_zero_gc(cloud_io::remote*, cloud_storage_clients::bucket_name);

    // Request that GC be started or stopped. These can be called in any order
    // and the last request will eventually take effect.
    void start();
    void stop();

    // Request and wait for GC to be completely stopped. After calling shutdown,
    // calling start() or stop will have no effect.
    seastar::future<> shutdown();

private:
    seastar::abort_source asrc_;
    std::unique_ptr<object_storage> storage_;
    std::unique_ptr<epoch_source> epoch_source_;
    bool should_run_{false};
    bool should_exit_{false};
    seastar::condition_variable worker_cv_;

    ssx::semaphore worker_sem_;
    seastar::future<> worker_;
    seastar::future<> worker();
    seastar::future<> gc();
    seastar::lowres_clock::time_point last_gc_;
};

} // namespace cloud_topics
