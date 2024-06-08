load("//bazel:build.bzl", "redpanda_cc_library")

redpanda_cc_library(
    name = "bytes",
    srcs = [
        "iobuf.cc",
    ],
    hdrs = [
        "include/bytes/bytes.h",
        "include/bytes/details/io_allocation_size.h",
        "include/bytes/details/io_byte_iterator.h",
        "include/bytes/details/io_fragment.h",
        "include/bytes/details/io_iterator_consumer.h",
        "include/bytes/details/io_placeholder.h",
        "include/bytes/details/out_of_range.h",
        "include/bytes/hash.h",
        "include/bytes/iobuf.h",
        "include/bytes/iostream.h",
        "include/bytes/scattered_message.h",
        "include/bytes/streambuf.h",
    ],
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
    deps = [
        "//src/v/base",
        "//src/v/container:intrusive",
    ],
)

redpanda_cc_library(
    name = "iobuf_parser",
    hdrs = [
        "include/bytes/iobuf_parser.h",
    ],
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
    deps = [
        ":bytes",
        "//src/v/strings",
        "//src/v/utils:vint",
    ],
)
