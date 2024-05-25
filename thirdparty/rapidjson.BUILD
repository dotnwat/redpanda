load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake")

filegroup(
    name = "rapidjson_srcs",
    srcs = glob(["**"]),
)

cmake(
    name = "librapidjson",
    lib_source = ":rapidjson_srcs",
    out_headers_only = True,
    cache_entries = {
        "RAPIDJSON_BUILD_EXAMPLES": "OFF",
        "RAPIDJSON_BUILD_TESTS": "OFF",
        "RAPIDJSON_BUILD_DOC": "OFF",
        "RAPIDJSON_HAS_STDSTRING": "ON",
    },
    visibility = [
        "//visibility:public",
    ],
)
