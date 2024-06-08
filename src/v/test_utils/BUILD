load("//bazel:build.bzl", "redpanda_cc_library")

redpanda_cc_library(
    name = "gtest",
    srcs = [
        "gtest_main.cc",
        "gtest_utils.cc",
    ],
    hdrs = [
        "gtest_utils.h",
        "randoms.h",
        "test.h",
    ],
    include_prefix = "test_utils",
    visibility = ["//visibility:public"],
    deps = [
        "//src/v/base",
        "//src/v/container:fragmented_vector",
        "//src/v/random",
        "//src/v/utils:tristate",
        "@googletest//:gtest",
        "@seastar//:testing",
        "@abseil-cpp//absl/container:node_hash_map",
        "@abseil-cpp//absl/container:node_hash_set",
        "@abseil-cpp//absl/container:btree",
    ],
)

redpanda_cc_library(
    name = "seastar_boost",
    srcs = [
        "seastar_testing_main.cc",
    ],
    hdrs = [
    ],
    include_prefix = "test_utils",
    visibility = ["//visibility:public"],
    deps = [
        "@seastar//:testing",
    ],
)
