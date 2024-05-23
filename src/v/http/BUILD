cc_library(
    name = "http",
    srcs = [
        "chunk_encoding.cc",
        "client.cc",
        "iobuf_body.cc",
        "logger.cc",
    ],
    hdrs = [
        "include/http/chunk_encoding.h",
        "include/http/client.h",
        "include/http/iobuf_body.h",
        "include/http/logger.h",
        "include/http/probe.h",
    ],
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//src/v/base",
        "//src/v/bytes",
        "@seastar//:libseastar",
    ],
)
