cc_library(
    name = "application",
    srcs = [
        "application.cc",
    ],
    hdrs = [
        "application.h",
        "monitor_unsafe_log_flag.h",
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
    ],
)

cc_binary(
    name = "redpanda",
    srcs = [
        "main.cc",
    ],
    deps = [
        ":application",
    ],
)
