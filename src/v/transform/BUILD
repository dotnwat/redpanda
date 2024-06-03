load("//src/v/rpc:compiler.bzl", "rpc_gen")

rpc_gen(
    "rpc_gen",
    src = "rpc/rpc.json",
    out = "rpc/include/transform/rpc/rpc_service.h",
)

cc_library(
    name = "rpc",
    srcs = [
        "rpc/client.cc",
        "rpc/deps.cc",
        "rpc/logger.cc",
        "rpc/serde.cc",
        "rpc/service.cc",
        "rpc/logger.h",
    ],
    hdrs = [
        "rpc/include/transform/rpc/client.h",
        "rpc/include/transform/rpc/deps.h",
        "rpc/include/transform/rpc/serde.h",
        "rpc/include/transform/rpc/service.h",
        "rpc/include/transform/rpc/rpc_service.h",
    ],
    includes = [
        "rpc/include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//src/v/base",
        "//src/v/model",
        "//src/v/config",
        "//src/v/cluster",
        "//src/v/metrics",
        "//src/v/kafka/server:kafka",
        "//src/v/storage:record_batch_builder",
        "@seastar//:libseastar",
    ],
)

cc_library(
    name = "logging",
    srcs = [
        "logging/event.cc",
        "logging/log_manager.cc",
        "logging/logger.cc",
        "logging/probes.cc",
        "logging/record_batcher.cc",
        "logging/rpc_client.cc",
        "logging/event.h",
        "logging/logger.h",
        "logging/probes.h",
        "logging/record_batcher.h",
    ],
    hdrs = [
        "logging/include/transform/logging/errc.h",
        "logging/include/transform/logging/fwd.h",
        "logging/include/transform/logging/io.h",
        "logging/include/transform/logging/log_manager.h",
        "logging/include/transform/logging/rpc_client.h",
    ],
    includes = [
        "logging/include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//src/v/base",
        "//src/v/model",
        "//src/v/config",
        "//src/v/metrics",
        "//src/v/storage:record_batch_builder",
        "@seastar//:libseastar",
        ":rpc",
    ],
)

cc_library(
    name = "transform",
    srcs = [
        "api.cc",
        "probe.cc",
        "logger.cc",
        "transform_processor.cc",
        "transform_manager.cc",
        "commit_batcher.cc",
        "txn_reader.cc",
        "memory_limiter.cc",
        "commit_batcher.h",
        "io.h",
        "logger.h",
        "memory_limiter.h",
        "probe.h",
        "transfer_queue.h",
        "transform_logger.h",
        "transform_manager.h",
        "transform_processor.h",
        "txn_reader.h",
    ],
    hdrs = [
        "include/transform/api.h",
        "include/transform/fwd.h",
    ],
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//src/v/base",
        "//src/v/model",
        "//src/v/kafka/server:kafka",
        "//src/v/wasm",
        "@seastar//:libseastar",
    ],
)
