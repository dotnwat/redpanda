cc_library(
    name = "bytes",
    srcs = [
        "iobuf.cc",
    ],
    hdrs = [
        "include/bytes/bytes.h",
        "include/bytes/hash.h",
        "include/bytes/iobuf.h",
        "include/bytes/iobuf_parser.h",
        "include/bytes/iostream.h",
        "include/bytes/scattered_message.h",
        "include/bytes/streambuf.h",
        "include/bytes/details/io_allocation_size.h",
        "include/bytes/details/io_byte_iterator.h",
        "include/bytes/details/io_fragment.h",
        "include/bytes/details/io_iterator_consumer.h",
        "include/bytes/details/io_placeholder.h",
        "include/bytes/details/out_of_range.h",
    ],
    includes = [
        "include",
    ],
    deps = [
        "@seastar//:libseastar",
        "//src/v/base:base",
        "//src/v/container:container",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "bytes_random",
    srcs = [
        "iobuf.cc",
    ],
    hdrs = [
        "include/bytes/random.h",
    ],
    includes = [
        "include",
    ],
    deps = [
        "@seastar//:libseastar",
        "//src/v/base:base",
        "//src/v/bytes:bytes",
        "//src/v/random:random",
    ],
    visibility = ["//visibility:public"],
)
