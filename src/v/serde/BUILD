cc_library(
    name = "serde",
    hdrs = glob([
        "*.h",
        "rw/*.h",
    ]),
    copts = [
    ],
    include_prefix = "serde",
    visibility = ["//visibility:public"],
    deps = [
        "//src/v/bytes:bytes",
        "//src/v/hashing:hashing",
        "//src/v/ssx:ssx",
        "//src/v/reflection:reflection",
        "//src/v/utils:utils",
    ],
)
