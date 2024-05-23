cc_library(
    name = "io",
    srcs = [
        "persistence.cc",
        "page.cc",
        "page_set.cc",
        "logger.cc",
        "io_queue.cc",
        "scheduler.cc",
        "page_cache.cc",
        "pager.cc",
        "paging_data_source.cc",
        "logger.h",
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
    deps = [
        "@seastar//:libseastar",
        "//src/v/base:base",
        "//src/v/container:container",
        "//src/v/ssx:ssx",
    ],
    visibility = ["//visibility:public"],
)
