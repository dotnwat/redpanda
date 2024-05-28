cc_library(
    name = "config",
    hdrs = [
        "config.h",
        "gssapi_principal_mapper.h",
        "oidc_url_parser.h",
    ],
    include_prefix = "security",
    visibility = ["//visibility:public"],
)

cc_library(
    name = "license",
    srcs = [
        "license.cc",
    ],
    hdrs = [
        "license.h",
    ],
    include_prefix = "security",
    visibility = ["//visibility:public"],
    deps = [
        "//src/v/base",
        "//src/v/serde",
        "//src/v/json",
    ]
)
