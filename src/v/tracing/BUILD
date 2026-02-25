load("//bazel:build.bzl", "redpanda_cc_library")

package(default_visibility = ["//visibility:public"])

redpanda_cc_library(
    name = "trace_context",
    hdrs = [
        "trace_context.h",
    ],
    deps = [
        "//src/v/random:generators",
        "//src/v/utils:uuid",
    ],
)

redpanda_cc_library(
    name = "span_buffer",
    srcs = [
        "span_buffer.cc",
    ],
    hdrs = [
        "span_buffer.h",
    ],
    deps = [
        ":trace_context",
        "//src/v/base",
        "//src/v/container:chunked_vector",
        "@seastar",
    ],
)

redpanda_cc_library(
    name = "span",
    srcs = [
        "span.cc",
    ],
    hdrs = [
        "span.h",
    ],
    deps = [
        ":span_buffer",
        ":trace_context",
        "@seastar",
    ],
)

redpanda_cc_library(
    name = "otlp_serializer",
    srcs = [
        "otlp_serializer.cc",
    ],
    hdrs = [
        "otlp_serializer.h",
    ],
    deps = [
        ":span_buffer",
        "//src/v/bytes:iobuf",
        "//src/v/container:chunked_vector",
    ],
)

redpanda_cc_library(
    name = "otlp_exporter",
    srcs = [
        "otlp_exporter.cc",
    ],
    hdrs = [
        "otlp_exporter.h",
    ],
    deps = [
        ":otlp_serializer",
        ":span_buffer",
        "//src/v/bytes:iobuf",
        "//src/v/config",
        "//src/v/http:http",
        "//src/v/ssx:future_util",
        "//src/v/utils:prefix_logger",
        "//src/v/utils:unresolved_address",
        "@boost//:beast",
        "@seastar",
    ],
)
