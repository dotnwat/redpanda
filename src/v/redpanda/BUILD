load("//src/v/redpanda:json2code.bzl", "json2code")
load("//src/v/redpanda:json2code.bzl", "json2code_def")

json2code(
    "cluster_config",
    src = "admin/api-doc/cluster_config.json",
    out = "admin/api-doc/cluster_config.json.hh")

json2code(
    "broker",
    src = "admin/api-doc/broker.json",
    out = "admin/api-doc/broker.json.hh")

json2code_def("config", src = "admin/api-doc/config.json",
    out = "admin/api-doc/config.json.hh",
    defn = "admin/api-doc/config.def.json",
)

json2code_def("security",
src = "admin/api-doc/security.json",
out = "admin/api-doc/security.json.hh",
defn = "admin/api-doc/security.def.json",)

json2code("raft", src = "admin/api-doc/raft.json", out = "admin/api-doc/raft.json.hh")
json2code("status", src = "admin/api-doc/status.json", out = "admin/api-doc/status.json.hh")
json2code("features", src = "admin/api-doc/features.json", out = "admin/api-doc/features.json.hh")
json2code("partition", src = "admin/api-doc/partition.json", out = "admin/api-doc/partition.json.hh")
json2code("hbadger", src = "admin/api-doc/hbadger.json", out = "admin/api-doc/hbadger.json.hh")
json2code("transaction", src = "admin/api-doc/transaction.json", out = "admin/api-doc/transaction.json.hh")
json2code("cluster", src = "admin/api-doc/cluster.json", out = "admin/api-doc/cluster.json.hh")
json2code("debug", src = "admin/api-doc/debug.json", out = "admin/api-doc/debug.json.hh")
json2code("usage", src = "admin/api-doc/usage.json", out = "admin/api-doc/usage.json.hh")
json2code("shadow_indexing", src = "admin/api-doc/shadow_indexing.json", out = "admin/api-doc/shadow_indexing.json.hh")
json2code("transform", src = "admin/api-doc/transform.json", out = "admin/api-doc/transform.json.hh")
json2code("recovery", src = "admin/api-doc/recovery.json", out = "admin/api-doc/recovery.json.hh")

cc_library(
    name = "application",
    srcs = [
        "application.cc",
        "cluster_config_schema_util.cc",
        "admin/server.cc",
        "admin/debug.cc",
        "admin/transform.cc",
        "admin/security.cc",
        "admin/recovery.cc",
        "admin/transaction.cc",
        "admin/partition.cc",
        "admin/usage.cc",
        "admin/kafka.cc",
        "admin/util.cc",
        "cli_parser.cc",
        "monitor_unsafe_log_flag.cc",
        "admin/api-doc/cluster_config.json.cc",
        "admin/api-doc/broker.json.cc",
        "admin/api-doc/config.json.cc",
        "admin/api-doc/raft.json.cc",
        "admin/api-doc/security.json.cc",
        "admin/api-doc/status.json.cc",
        "admin/api-doc/features.json.cc",
        "admin/api-doc/partition.json.cc",
        "admin/api-doc/hbadger.json.cc",
        "admin/api-doc/transaction.json.cc",
        "admin/api-doc/cluster.json.cc",
        "admin/api-doc/debug.json.cc",
        "admin/api-doc/usage.json.cc",
        "admin/api-doc/shadow_indexing.json.cc",
        "admin/api-doc/transform.json.cc",
        "admin/api-doc/recovery.json.cc",
    ],
    hdrs = [
        "application.h",
        "monitor_unsafe_log_flag.h",
        "cluster_config_schema_util.h",
        "cli_parser.h",
        "admin/api-doc/cluster_config.json.hh",
        "admin/api-doc/broker.json.hh",
        "admin/api-doc/config.json.hh",
        "admin/api-doc/raft.json.hh",
        "admin/api-doc/security.json.hh",
        "admin/api-doc/status.json.hh",
        "admin/api-doc/features.json.hh",
        "admin/api-doc/partition.json.hh",
        "admin/api-doc/hbadger.json.hh",
        "admin/api-doc/transaction.json.hh",
        "admin/api-doc/cluster.json.hh",
        "admin/api-doc/debug.json.hh",
        "admin/api-doc/usage.json.hh",
        "admin/api-doc/shadow_indexing.json.hh",
        "admin/api-doc/transform.json.hh",
        "admin/api-doc/recovery.json.hh",
        "admin/server.h",
        "admin/util.h",
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
        "//src/v/migrations",
        "@re2",
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
