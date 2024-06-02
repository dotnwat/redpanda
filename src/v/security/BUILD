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
        "//src/v/json",
        "//src/v/serde",
    ],
)

cc_library(
    name = "audit_log_manager",
    srcs = [
    ],
    hdrs = [
        "audit/audit_log_manager.h",
    ],
    include_prefix = "security",
    visibility = ["//visibility:public"],
    deps = [
        "//src/v/kafka/client",
    ],
)

cc_library(
    name = "security",
    srcs = [
    ],
    hdrs = [
        "acl.h",
        "acl_entry_set.h",
        "acl_store.h",
        "audit/client_probe.h",
        "audit/logger.h",
        "audit/probe.h",
        "audit/schemas/application_activity.h",
        "audit/schemas/hashing_utils.h",
        "audit/schemas/iam.h",
        "audit/schemas/schemas.h",
        "audit/schemas/types.h",
        "audit/schemas/utils.h",
        "audit/types.h",
        "authorizer.h",
        "credential_store.h",
        "ephemeral_credential.h",
        "ephemeral_credential_store.h",
        "errc.h",
        "exceptions.h",
        "fwd.h",
        "gssapi_authenticator.h",
        "krb5_configurator.h",
        "logger.h",
        "mtls.h",
        "oidc_authenticator.h",
        "oidc_service.h",
        "request_auth.h",
        "role.h",
        "role_store.h",
        "sasl_authentication.h",
        "scram_algorithm.h",
        "scram_authenticator.h",
        "scram_credential.h",
        "types.h",
    ],
    include_prefix = "security",
    visibility = ["//visibility:public"],
    deps = [
        "//src/v/base",
        "//src/v/bytes:bytes_random",
        "//src/v/json",
        "//src/v/serde",
    ],
)
