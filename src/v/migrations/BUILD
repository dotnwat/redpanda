cc_library(
    name = "migrations",
    srcs = [
        "cloud_storage_config.cc",
        "feature_migrator.cc",
        "rbac_migrator.cc",
    ],
    hdrs = [
        "cloud_storage_config.h",
        "feature_migrator.h",
        "migrators.h",
        "rbac_migrator.h",
    ],
    include_prefix = "migrations",
    visibility = ["//visibility:public"],
    deps = [
        "//src/v/cluster",
    ],
)
