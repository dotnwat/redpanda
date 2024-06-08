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
    ],
)
