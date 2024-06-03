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
        "rpc/logger.h",
        "rpc/serde.cc",
        "rpc/service.cc",
    ],
    hdrs = [
        "rpc/include/transform/rpc/client.h",
        "rpc/include/transform/rpc/deps.h",
        "rpc/include/transform/rpc/rpc_service.h",
        "rpc/include/transform/rpc/serde.h",
        "rpc/include/transform/rpc/service.h",
    ],
    includes = [
        "rpc/include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":stm",
        "//src/v/base",
        "//src/v/cluster",
        "//src/v/config",
        "//src/v/kafka/server:kafka",
        "//src/v/metrics",
        "//src/v/model",
        "//src/v/storage:record_batch_builder",
        "@seastar//:libseastar",
    ],
)

cc_library(
    name = "stm",
    srcs = [
        "stm/transform_offsets_stm.cc",
    ],
    hdrs = [
        "stm/include/transform/stm/transform_offsets_stm.h",
    ],
    includes = [
        "stm/include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//src/v/base",
        "//src/v/cluster",
        "//src/v/model",
        "@seastar//:libseastar",
    ],
)

cc_library(
    name = "logging",
    srcs = [
        "logging/event.cc",
        "logging/event.h",
        "logging/log_manager.cc",
        "logging/logger.cc",
        "logging/logger.h",
        "logging/probes.cc",
        "logging/probes.h",
        "logging/record_batcher.cc",
        "logging/record_batcher.h",
        "logging/rpc_client.cc",
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
        ":rpc",
        "//src/v/base",
        "//src/v/config",
        "//src/v/metrics",
        "//src/v/model",
        "//src/v/storage:record_batch_builder",
        "@seastar//:libseastar",
    ],
)

cc_library(
    name = "transform",
    srcs = [
        "api.cc",
        "commit_batcher.cc",
        "commit_batcher.h",
        "io.h",
        "logger.cc",
        "logger.h",
        "memory_limiter.cc",
        "memory_limiter.h",
        "probe.cc",
        "probe.h",
        "transfer_queue.h",
        "transform_logger.h",
        "transform_manager.cc",
        "transform_manager.h",
        "transform_processor.cc",
        "transform_processor.h",
        "txn_reader.cc",
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
        ":logging",
        "//src/v/base",
        "//src/v/kafka/server:kafka",
        "//src/v/model",
        "//src/v/wasm",
        "@seastar//:libseastar",
    ],
)
