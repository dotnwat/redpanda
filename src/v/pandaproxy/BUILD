load("//src/v/redpanda:json2code.bzl", "json2code")

json2code("rest",
    src = "api/api-doc/rest.json",
    out = "api/api-doc/rest.json.hh",
)

#defn = "admin/api-doc/config.def.json",

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
        "schema_registry/error.h",
        "schema_registry/exceptions.h",
        "schema_registry/fwd.h",
        "schema_registry/seq_writer.h",
        "schema_registry/service.h",
        "schema_registry/sharded_store.h",
        "schema_registry/types.h",
        "schema_registry/validation.h",
    ],
    include_prefix = "pandaproxy",
    visibility = ["//visibility:public"],
    deps = [
        ":logger",
        ":pandaproxy",
        ":server",
        "//src/v/kafka/client",
        "@avro//:libavro",
    ],
)

cc_library(
    name = "server",
    hdrs = [
        "server.h",
    ],
    include_prefix = "pandaproxy",
    visibility = ["//visibility:public"],
    deps = [
        "//src/v/security:request_auth",
    ],
)

cc_library(
    name = "pandaproxy",
    srcs = [
        "error.cc",
        "logger.cc",
        "probe.cc",
        "server.cc",
        "kafka_client_cache.cc",
        "rest/api.cc",
        "rest/configuration.cc",
        "rest/handlers.cc",
        "rest/proxy.cc",
    ],
    hdrs = [
        "fwd.h",
        "json/types.h",
        "json/error.h",
        "json/exceptions.h",
        "json/requests/brokers.h",
        "json/requests/fetch.h",
        "json/requests/produce.h",
        "json/requests/offset_commit.h",
        "json/requests/partition_offsets.h",
        "json/requests/offset_fetch.h",
        "json/requests/partitions.h",
        "json/requests/subscribe_consumer.h",
        "json/requests/create_consumer.h",
        "json/requests/error_reply.h",
        "json/rjson_util.h",
        "json/rjson_parse.h",
        "json/iobuf.h",
        "parsing/error.h",
        "parsing/exceptions.h",
        "parsing/httpd.h",
        "parsing/from_chars.h",
        "kafka_client_cache.h",
        "schema_registry/exceptions.h",
        "rest/configuration.h",
        "rest/api.h",
        "rest/fwd.h",
        "rest/handlers.h",
        "rest/proxy.h",
        "types.h",
        "util.h",
        "probe.h",
        "logger.h",
        "error.h",
        "reply.h",
        "api/api-doc/rest.json.hh",
    ],
    include_prefix = "pandaproxy",
    visibility = ["//visibility:public"],
    deps = [
        "//src/v/metrics",
        "//src/v/config",
        "//src/v/cluster",
        "//src/v/kafka/protocol:types",
        "//src/v/kafka/client",
        "//src/v/json",
        ":server",
    ],
)
