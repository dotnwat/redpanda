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
#include "model/fundamental.h"
#include "ssx/semaphore.h"

#include <seastar/core/condition-variable.hh>
#include <seastar/core/future.hh>

namespace cloud_io {
class remote;
}

namespace cloud_topics {

class level_zero_gc {
    // Avoid thrashing (e.g. cause by leadership flapping) by requiring a
    // minimum delay between GC worker activations.
    static constexpr std::chrono::seconds min_period{5};

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

        virtual seastar::future<cloud_io::list_result> list_objects() = 0;
        // virtual seastar::future<> delete_objects() = 0;
        //  int remote::delete_objects_max_keys() const {
        //  bool is_batch_delete_supported() const;
    };

    // Construct using the given storage provider
    explicit level_zero_gc(std::unique_ptr<object_storage>);

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
    std::unique_ptr<object_storage> storage_;
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
