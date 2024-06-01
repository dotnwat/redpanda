py_binary(
    name = "rpc_compiler",
    srcs = ["rpc_compiler.py"],
    visibility = ["//visibility:public"],
    deps = [
        "@my_deps//jinja2",
    ],
)

cc_library(
    name = "rpc",
    srcs = [
        "connection_cache.cc",
        "connection_set.cc",
        "netbuf.cc",
        "reconnect_transport.cc",
        "rpc_server.cc",
        "rpc_utils.cc",
        "transport.cc",
        "types.cc",
    ],
    hdrs = [
        "include/rpc/backoff_policy.h",
        "include/rpc/connection_cache.h",
        "include/rpc/connection_set.h",
        "include/rpc/errc.h",
        "include/rpc/exceptions.h",
        "include/rpc/fwd.h",
        "include/rpc/logger.h",
        "include/rpc/parse_utils.h",
        "include/rpc/reconnect_transport.h",
        "include/rpc/response_handler.h",
        "include/rpc/rpc_server.h",
        "include/rpc/rpc_utils.h",
        "include/rpc/service.h",
        "include/rpc/transport.h",
        "include/rpc/types.h",
    ],
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//src/v:version",
        "//src/v/base",
        "//src/v/bytes",
        "//src/v/compression",
        "//src/v/config",
        "//src/v/metrics",
        "//src/v/net",
        "//src/v/ssx",
    ],
)
