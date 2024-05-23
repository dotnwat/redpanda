cc_library(
    name = "ssx",
    srcs = [
        "work_queue.cc",
    ],
    hdrs = [
        "include/ssx/abort_source.h",
        "include/ssx/async-clear.h",
        "include/ssx/async_algorithm.h",
        "include/ssx/event.h",
        "include/ssx/future-util.h",
        "include/ssx/fwd.h",
        "include/ssx/semaphore.h",
        "include/ssx/sformat.h",
        "include/ssx/sharded_ptr.h",
        "include/ssx/sleep_abortable.h",
        "include/ssx/task_local_ptr.h",
        "include/ssx/thread_worker.h",
        "include/ssx/watchdog.h",
        "include/ssx/work_queue.h",
    ],
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//src/v/base",
        "//src/v/utils",
        "@seastar//:libseastar",
    ],
)
