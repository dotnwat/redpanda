/*
 * Copyright 2025 Redpanda Data, Inc.
 *
 * Licensed as a Redpanda Enterprise file under the Redpanda Community
 * License (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 *
 * https://github.com/redpanda-data/redpanda/blob/master/licenses/rcl.md
 */
#include "cloud_topics/level_zero/gc/level_zero_gc.h"

#include <gtest/gtest.h>

class object_storage_test_impl
  : public cloud_topics::level_zero_gc::object_storage {
public:
    object_storage_test_impl() = default;

    seastar::future<std::expected<
      cloud_storage_clients::client::list_bucket_result,
      cloud_storage_clients::error_outcome>>
    list_objects() override {
        co_return cloud_storage_clients::client::list_bucket_result{};
    }

    seastar::future<std::expected<void, cloud_io::upload_result>>
    delete_objects(
      std::vector<cloud_storage_clients::client::list_bucket_item>) override {
        co_return std::expected<void, cloud_io::upload_result>();
    }
};

class epoch_source_test_impl
  : public cloud_topics::level_zero_gc::epoch_source {
public:
    seastar::future<
      std::expected<std::optional<cloud_topics::cluster_epoch>, std::string>>
    max_gc_eligible_epoch() override {
        co_return std::nullopt;
    }
};

TEST(Foo, Bar) {}
