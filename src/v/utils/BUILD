cc_library(
    name = "retry_chain_node",
    srcs = [
        "retry_chain_node.cc",
    ],
    hdrs = [
        "retry_chain_node.h",
    ],
    include_prefix = "utils",
    visibility = ["//visibility:public"],
    deps = [
        "//src/v/base",
        "//src/v/ssx",
        "//src/v/random",
    ],
)

cc_library(
    name = "unresolved_address",
    hdrs = [
        "unresolved_address.h",
    ],
    include_prefix = "utils",
    visibility = ["//visibility:public"],
    deps = [
        "//src/v/serde",
    ],
)

cc_library(
    name = "tracking_allocator",
    srcs = [
        "tracking_allocator.cc",
    ],
    hdrs = [
        "tracking_allocator.h",
    ],
    include_prefix = "utils",
    visibility = ["//visibility:public"],
    deps = [
        "//src/v/json",
    ],
)

cc_library(
    name = "xid",
    srcs = [
        "xid.cc",
    ],
    hdrs = [
        "xid.h",
    ],
    include_prefix = "utils",
    visibility = ["//visibility:public"],
    deps = [
        "//src/v/serde",
    ],
)

cc_library(
    name = "utils",
    srcs = [
        "base64.cc",
        "bottomless_token_bucket.cc",
        "directory_walker.cc",
        "file_io.cc",
        "hdr_hist.cc",
        "human.cc",
        "log_hist.cc",
        "uuid.cc",
        "vint.cc",
    ],
    hdrs = [
        "absl_sstring_hash.h",
        "adjustable_semaphore.h",
        "auto_fmt.h",
        "available_promise.h",
        "base64.h",
        "bottomless_token_bucket.h",
        "delta_for.h",
        "directory_walker.h",
        "ema.h",
        "exceptions.h",
        "expiring_promise.h",
        "file_io.h",
        "filtered_lower_bound.h",
        "fixed_string.h",
        "functional.h",
        "hdr_hist.h",
        "human.h",
        "inet_address_wrapper.h",
        "log_hist.h",
        "move_canary.h",
        "moving_average.h",
        "mutex.h",
        "named_type.h",
        "notification_list.h",
        "object_pool.h",
        "prefix_logger.h",
        "queue_depth_control.h",
        "remote.h",
        "retry.h",
        "rwlock.h",
        "stable_iterator_adaptor.h",
        "static_deleter_fn.h",
        "stop_signal.h",
        "stream_utils.h",
        "to_string.h",
        "token_bucket.h",
        "tristate.h",
        "truncating_logger.h",
        "uuid.h",
        "vint.h",
        "waiter_queue.h",
        "vint_iostream.h",
    ],
    copts = [
    ],
    include_prefix = "utils",
    visibility = ["//visibility:public"],
    deps = [
        "//src/v/base",
        "//src/v/bytes",
        "@base64//:libbase64",
        "@hdrhistogram//:libhdrhistogram",
    ],
)
