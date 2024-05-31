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

cc_library(
    name = "security",
    srcs = [
    ],
    hdrs = [
        "acl.h",
        "role.h",
        "types.h",
        "scram_credential.h",
        "fwd.h",
        "credential_store.h",
        "authorizer.h",
        "ephemeral_credential.h",
        "acl_entry_set.h",
        "logger.h",
        "ephemeral_credential_store.h",
        "oidc_service.h",
        "acl_store.h",
        "role_store.h",
        "scram_algorithm.h",
        "scram_authenticator.h",
        "sasl_authentication.h",
        "audit/schemas/types.h",
        "exceptions.h",
        "errc.h",
    ],
    include_prefix = "security",
    visibility = ["//visibility:public"],
    deps = [
        "//src/v/base",
        "//src/v/serde",
        "//src/v/json",
        "//src/v/bytes:bytes_random",
    ]
)
