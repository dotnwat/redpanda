load("//bazel:build.bzl", "redpanda_cc_library")

package(
    default_visibility = [
        "//src/v/io/tests:__pkg__",
    ],
)

redpanda_cc_library(
    name = "persistence",
    srcs = [
        "persistence.cc",
    ],
    hdrs = [
        "persistence.h",
    ],
    include_prefix = "io",
    deps = [
        "//src/v/base",
        "@abseil-cpp//absl/container:btree",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@seastar",
    ],
)

redpanda_cc_library(
    name = "io",
    srcs = [
        "io_queue.cc",
        "logger.cc",
        "logger.h",
        "page.cc",
        "page_cache.cc",
        "page_set.cc",
        "pager.cc",
        "paging_data_source.cc",
        "scheduler.cc",
    ],
    hdrs = [
        "cache.h",
        "interval_map.h",
        "io_queue.h",
        "page.h",
        "page_cache.h",
        "page_set.h",
        "pager.h",
        "paging_data_source.h",
        "scheduler.h",
    ],
    include_prefix = "io",
    visibility = ["//visibility:public"],
    implementation_deps = [
        ":persistence",
    ],
    deps = [
        "//src/v/base",
        "//src/v/container:intrusive",
        "//src/v/ssx:future_util",
        "//src/v/ssx:semaphore",
        "@abseil-cpp//absl/container:btree",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@boost//:intrusive",
        "@boost//:iterator",
        "@fmt",
        "@seastar",
    ],
)
