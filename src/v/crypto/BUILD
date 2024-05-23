cc_library(
    name = "crypto",
    srcs = [
        "crypto.cc",
        "digest.cc",
        "hmac.cc",
        "key.cc",
        "random.cc",
        "signature.cc",
        "ssl_utils.cc",
        "ossl_context_service.cc",
        "internal.h",
        "key.h",
        "ssl_utils.h",
    ],
    hdrs = [
        "include/crypto/crypto.h",
        "include/crypto/exceptions.h",
        "include/crypto/ossl_context_service.h",
        "include/crypto/types.h",
    ],
    includes = [
        "include",
    ],
    deps = [
        "//src/v/base:base",
        "//src/v/bytes:bytes",
        "//src/v/ssx:ssx",
    ],
    visibility = ["//visibility:public"],
)
