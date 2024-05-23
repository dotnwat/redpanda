cc_library(
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
        "include/bytes/iobuf_parser.h",
        "include/bytes/iostream.h",
        "include/bytes/scattered_message.h",
        "include/bytes/streambuf.h",
    ],
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//src/v/base",
        "//src/v/container",
        "//src/v/strings:strings",
        "@seastar//:libseastar",
    ],
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
    visibility = ["//visibility:public"],
    deps = [
        "//src/v/base",
        "//src/v/bytes",
        "//src/v/random",
        "@seastar//:libseastar",
    ],
)
