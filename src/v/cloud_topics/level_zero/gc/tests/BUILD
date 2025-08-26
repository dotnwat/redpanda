load("//bazel:test.bzl", "redpanda_cc_gtest")

redpanda_cc_gtest(
    name = "level_zero_gc_test",
    timeout = "short",
    srcs = [
        "level_zero_gc_tests.cc",
    ],
    deps = [
        "//src/v/bytes:iostream",
        "//src/v/cloud_topics/level_zero/gc:level_zero_gc",
        "//src/v/model/tests:random",
        "//src/v/test_utils:gtest",
        "@googletest//:gtest",
        "@seastar",
    ],
)
