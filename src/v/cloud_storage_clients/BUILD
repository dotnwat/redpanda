cc_library(
    name = "cloud_storage_clients",
    srcs = [
        "abs_client.cc",
        "abs_error.cc",
        "client_pool.cc",
        "client_probe.cc",
        "configuration.cc",
        "s3_client.cc",
        "s3_error.cc",
        "util.cc",
        "xml_sax_parser.cc",
    ],
    hdrs = [
        "abs_client.h",
        "client.h",
        "xml_sax_parser.h",
    ],
    include_prefix = "cloud_storage_clients",
    visibility = ["//visibility:public"],
    deps = [
        "//src/v/cloud_roles",
        "@seastar//:libseastar",
    ],
)
