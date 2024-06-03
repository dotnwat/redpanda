load("//src/v/redpanda:json2code.bzl", "json2code")

json2code(
    "cluster_config",
    src = "admin/api-doc/cluster_config.json",
    out = "admin/api-doc/cluster_config.json.hh")

cc_library(
    name = "application",
    srcs = [
        "application.cc",
        "cluster_config_schema_util.cc",
        "admin/server.cc",
        #"admin/debug.cc",
        #"admin/transform.cc",
        #"admin/security.cc",
        #"admin/recovery.cc",
        #"admin/transaction.cc",
        #"admin/partition.cc",
        #"admin/usage.cc",
        #"admin/kafka.cc",
        #"admin/util.cc",
        "cli_parser.cc",
        "monitor_unsafe_log_flag.cc",
    ],
    hdrs = [
        "application.h",
        "monitor_unsafe_log_flag.h",
        "cluster_config_schema_util.h",
        "cli_parser.h",
        "admin/api-doc/cluster_config.json.hh",
        "admin/server.h",
    ],
    include_prefix = "redpanda",
    visibility = ["//visibility:public"],
    deps = [
        "//src/v/cluster",
        "//src/v/kafka/client",
        "//src/v/kafka/server:kafka",
        "//src/v/metrics",
        "//src/v/net",
        "//src/v/pandaproxy",
        "//src/v/pandaproxy:schema_registry_interface",
        "//src/v/resource_mgmt",
        "//src/v/transform",
    ],
)

cc_binary(
    name = "redpanda",
    srcs = [
        "main.cc",
    ],
    deps = [
        ":application",
        "//src/v/syschecks",
    ],
)
