cc_library(
    name = "hashing",
    srcs = [
        "murmur.cc",
    ],
    hdrs = [
        "include/hashing/combine.h",
        "include/hashing/crc32.h",
        "include/hashing/crc32c.h",
        "include/hashing/jump_consistent_hash.h",
        "include/hashing/murmur.h",
        "include/hashing/secure.h",
        "include/hashing/xx.h",
    ],
    defines = [
        "XXH_PRIVATE_API",
    ],
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//src/v/base",
        "@crc32c//:crc32c",
    ],
)
