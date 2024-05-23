cc_library(
    name = "io",
    srcs = [
        "io_queue.cc",
        "logger.cc",
        "logger.h",
        "page.cc",
        "page_cache.cc",
        "page_set.cc",
        "pager.cc",
        "paging_data_source.cc",
        "persistence.cc",
        "scheduler.cc",
    ],
    hdrs = [
        "include/io/cache.h",
        "include/io/interval_map.h",
        "include/io/io_queue.h",
        "include/io/page.h",
        "include/io/page_cache.h",
        "include/io/page_set.h",
        "include/io/pager.h",
        "include/io/paging_data_source.h",
        "include/io/persistence.h",
        "include/io/scheduler.h",
    ],
    # use -I instead, apparently according to docs
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//src/v/base",
        "//src/v/container",
        "//src/v/ssx",
        "@seastar//:libseastar",
    ],
)
