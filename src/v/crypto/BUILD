cc_library(
    name = "crypto",
    srcs = [
        "crypto.cc",
        "digest.cc",
        "hmac.cc",
        "internal.h",
        "key.cc",
        "key.h",
        "ossl_context_service.cc",
        "random.cc",
        "signature.cc",
        "ssl_utils.cc",
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
    visibility = ["//visibility:public"],
    deps = [
        "//src/v/base",
        "//src/v/bytes",
        "//src/v/ssx",
    ],
)
