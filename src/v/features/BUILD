cc_library(
    name = "features",
    srcs = [
        "feature_table.cc",
        "feature_table_snapshot.cc",
        "logger.cc",
    ],
    hdrs = [
        "feature_state.h",
        "feature_table.h",
        "feature_table_snapshot.h",
        "fwd.h",
        "logger.h",
    ],
    include_prefix = "features",
    visibility = ["//visibility:public"],
    deps = [
        "//src/v:version",
        "//src/v/base",
        "//src/v/cluster:features",
        "//src/v/cluster:version",
        "//src/v/config",
        "//src/v/security:license",
        "//src/v/storage:record_batch_builder",
        "@seastar//:libseastar",
    ],
)
