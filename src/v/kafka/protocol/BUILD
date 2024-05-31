cc_library(
    name = "protocol",
    srcs = [
        "errors.cc",
        "logger.cc",
        "types.cc",
        "batch_reader.cc",
        "kafka_batch_adapter.cc",
    ],
    hdrs = [
        "types.h",
        "wire.h",
        "batch_reader.h",
        "kafka_batch_adapter.h",
        "logger.h",
        "errors.h",
        "exceptions.h",
        "legacy_message.h",
    ],
    include_prefix = "kafka/protocol",
    visibility = ["//visibility:public"],
    deps = [
        "//src/v/base",
        "//src/v/bytes",
        "//src/v/utils",
        "//src/v/ssx",
        "//src/v/model",
        "//src/v/storage:record_batch_builder",
        "//src/v/kafka:types",
        "@seastar//:libseastar",
    ],
)

# seems like this will go into the server
# "flex_versions.h",
# "flex_versions.cc",

    #${message_hdrs}
    #${msg_src}
