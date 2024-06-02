cc_library(
    name = "schema_registry_config",
    hdrs = [
        "schema_registry/schema_id_validation.h",
        "schema_registry/subject_name_strategy.h",
    ],
    include_prefix = "pandaproxy",
    visibility = ["//visibility:public"],
)

cc_library(
    name = "logger",
    hdrs = [
        "logger.h",
    ],
    include_prefix = "pandaproxy",
    visibility = ["//visibility:public"],
    deps = [
    ],
)

cc_library(
    name = "schema_registry_interface",
    hdrs = [
        "schema_registry/api.h",
        "schema_registry/configuration.h",
        "schema_registry/fwd.h",
        "schema_registry/types.h",
        "schema_registry/validation.h",
        "schema_registry/seq_writer.h",
        "schema_registry/error.h",
        "schema_registry/exceptions.h",
        "schema_registry/sharded_store.h",
        "schema_registry/service.h",
    ],
    include_prefix = "pandaproxy",
    visibility = ["//visibility:public"],
    deps = [
        "@avro//:libavro",
        "//src/v/kafka/client",
        ":logger",
        ":server",
        ":pandaproxy",
    ],
)

cc_library(
    name = "server",
    hdrs = [
        "server.h",
    ],
    include_prefix = "pandaproxy",
    visibility = ["//visibility:public"],
)

cc_library(
    name = "pandaproxy",
    hdrs = [
        "fwd.h",
        "rest/configuration.h",
        "rest/fwd.h",
        "json/types.h",
        "kafka_client_cache.h",
        "types.h",
        "util.h",
    ],
    include_prefix = "pandaproxy",
    visibility = ["//visibility:public"],
)
