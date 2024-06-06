load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake")
load("@rules_proto//proto:defs.bzl", "proto_library")

py_binary(
    name = "seastar-json2code",
    srcs = ["scripts/seastar-json2code.py"],
    visibility = ["//visibility:public"],
    deps = [
    ],
)

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
    name = "seastar",
    srcs = glob(
        [
            "src/**/*.cc",
            "src/**/*.hh",
        ],
        exclude = ["src/seastar.cc"],
    ),
    hdrs = glob([
        "include/**/*.hh",
    ]) + [
        "include/seastar/http/chunk_parsers.hh",
        "include/seastar/http/request_parser.hh",
        "include/seastar/http/response_parser.hh",
    ],
    copts = [
        "-std=c++20",
    ],
    defines = [
        "SEASTAR_API_LEVEL=6",
        "SEASTAR_SSTRING",
        "SEASTAR_SCHEDULING_GROUPS_COUNT=32",
        #        "BOOST_TEST_ALTERNATIVE_INIT_API",
        "BOOST_TEST_DYN_LINK",
        "BOOST_TEST_NO_LIB",
    ],
    includes = [
        "include",
        "src",
    ],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":metrics_cc_proto",
        "@boost//:filesystem",
        "@boost//:program_options",
        "@boost//:test.so",
        "@boost//:thread",
        "@boost//:algorithm",
        "@c-ares",
        "@fmt",
        "@gnutls",
        "@lksctp",
        "@lz4",
        "@protobuf",
        "@yaml-cpp",
    ],
)
