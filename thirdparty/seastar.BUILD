load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake")
load("@rules_proto//proto:defs.bzl", "proto_library")

genrule(
    name = "http_request_parser",
    srcs = ["src/http/request_parser.rl"],
    outs = ["include/seastar/http/request_parser.hh"],
    cmd = "ragel -G2 -o $@ $(SRCS)",
)

genrule(
    name = "http_response_parser",
    srcs = ["src/http/response_parser.rl"],
    outs = ["include/seastar/http/response_parser.hh"],
    cmd = "ragel -G2 -o $@ $(SRCS)",
)

genrule(
    name = "http_chunk_parsers",
    srcs = ["src/http/chunk_parsers.rl"],
    outs = ["include/seastar/http/chunk_parsers.hh"],
    cmd = "ragel -G2 -o $@ $(SRCS)",
)

proto_library(
    name = "metrics_proto",
    srcs = ["src/proto/metrics2.proto"],
    deps = ["@protobuf//:timestamp_proto"],
)

cc_proto_library(
    name = "metrics_cc_proto",
    deps = [":metrics_proto"],
)

cc_library(
    name = "libseastar",
    srcs = glob([
        "src/**/*.cc",
        "src/**/*.hh",
    ], exclude=["src/seastar.cc"]),
    defines = [
        "SEASTAR_API_LEVEL=6",
        "SEASTAR_SSTRING",
        "SEASTAR_SCHEDULING_GROUPS_COUNT=32",
        "BOOST_TEST_ALTERNATIVE_INIT_API",
    ],
    hdrs = glob([
        "include/**/*.hh",
    ]) + ["include/seastar/http/request_parser.hh",
    "include/seastar/http/response_parser.hh",
    "include/seastar/http/chunk_parsers.hh"],
    copts = [
        "-std=c++20",
    ],
    includes = [
        "include",
        "src",
    ],
    deps = [
        "@boost//:boost",
        "@fmt",
        "@gnutls//:libgnutls",
        "@yaml-cpp",
        "@protobuf",
        ":metrics_cc_proto",
        "@lksctp//:liblksctp",
        "@c-ares//:ares",
        "@lz4",
    ],
    visibility = [
        "//visibility:public",
    ],
)
