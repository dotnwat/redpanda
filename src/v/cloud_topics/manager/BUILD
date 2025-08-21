load("//bazel:build.bzl", "redpanda_cc_library")

redpanda_cc_library(
    name = "level_zero_gc",
    srcs = [
        "level_zero_gc.cc",
    ],
    hdrs = [
        "level_zero_gc.h",
    ],
    implementation_deps = [
        "//src/v/base",
        "//src/v/cloud_topics:logger",
        "//src/v/cluster",
        "//src/v/raft",
		"//src/v/cloud_io:remote",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//src/v/cluster:notification",
        "//src/v/model",
		"//src/v/ssx:semaphore",
        "@seastar",
    ],
)

redpanda_cc_library(
    name = "manager",
    srcs = [
        "manager.cc",
    ],
    hdrs = [
        "manager.h",
    ],
    implementation_deps = [
        "//src/v/base",
        "//src/v/cloud_topics:logger",
        "//src/v/cluster",
        "//src/v/raft",
    ],
    visibility = ["//visibility:public"],
    deps = [
		":level_zero_gc",
        "//src/v/cluster:notification",
        "//src/v/model",
        "@seastar",
    ],
)
