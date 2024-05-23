cc_library(
    name = "http",
    srcs = [
        "iobuf_body.cc",
        "chunk_encoding.cc",
        "client.cc",
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
    deps = [
        "@seastar//:libseastar",
        "//src/v/base:base",
        "//src/v/bytes:bytes",
    ],
    visibility = ["//visibility:public"],
)
