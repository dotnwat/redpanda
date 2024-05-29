cc_library(
    name = "finjector",
    srcs = [
        "hbadger.cc",
        "stress_fiber.cc",
    ],
    hdrs = [
        "hbadger.h",
        "stress_fiber.h",
    ],
    include_prefix = "finjector",
    visibility = ["//visibility:public"],
    deps = [
        "//src/v/base",
        "//src/v/random",
        "@seastar//:libseastar",
    ],
)
