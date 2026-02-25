load("//bazel:test.bzl", "redpanda_cc_gtest")

redpanda_cc_gtest(
    name = "span_test",
    timeout = "short",
    srcs = [
        "span_test.cc",
    ],
    deps = [
        "//src/v/test_utils:gtest",
        "//src/v/tracing:span",
        "//src/v/tracing:span_buffer",
        "//src/v/tracing:trace_context",
        "@googletest//:gtest",
        "@seastar",
    ],
)

redpanda_cc_gtest(
    name = "otlp_serializer_test",
    timeout = "short",
    srcs = [
        "otlp_serializer_test.cc",
    ],
    deps = [
        "//src/v/bytes:iobuf",
        "//src/v/test_utils:gtest",
        "//src/v/tracing:otlp_serializer",
        "//src/v/tracing:span_buffer",
        "//src/v/tracing:trace_context",
        "@googletest//:gtest",
        "@seastar",
    ],
)
