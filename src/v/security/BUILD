cc_library(
    name = "config",
    srcs = [
        "config_bsl.cc",
        "config_rcl.cc",
    ],
    hdrs = [
        "config.h",
        "oidc_url_parser.h",
    ],
    include_prefix = "security",
    visibility = ["//visibility:public"],
    deps = [
        "//src/v/base",
    ],
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
    name = "request_auth",
    srcs = [
        "request_auth.cc",
    ],
    hdrs = [
        "request_auth.h",
    ],
    include_prefix = "security",
    visibility = ["//visibility:public"],
    deps = [
        "//src/v/base",
        "//src/v/cluster",
    ],
)

cc_library(
    name = "security",
    srcs = [
        "acl.cc",
        "authorizer.cc",
        "credential.cc",
        "gssapi_authenticator.cc",
        "gssapi_principal_mapper.cc",
        "jwt.cc",
        "krb5.cc",
        "krb5_configurator.cc",
        "mtls.cc",
        "oidc_authenticator.cc",
        "oidc_principal_mapping.cc",
        "oidc_service.cc",
        "role.cc",
        "scram_algorithm.cc",
        "scram_authenticator.cc",
    ],
    hdrs = [
        "acl.h",
        "acl_entry_set.h",
        "acl_store.h",
        "gssapi_principal_mapper.h",
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
        "krb5.h",
        "logger.h",
        "mtls.h",
        "jwt.h",
        "oidc_authenticator.h",
        "oidc_service.h",
        "role.h",
        "role_store.h",
        "sasl_authentication.h",
        "scram_algorithm.h",
        "scram_authenticator.h",
        "scram_credential.h",
        "types.h",
        "oidc_principal_mapping.h",
        "oidc_error.h",
        "gssapi.h",
    ],
    include_prefix = "security",
    visibility = ["//visibility:public"],
    deps = [
        "//src/v:version",
        "//src/v/base",
        "//src/v/bytes:bytes_random",
        "//src/v/json",
        "//src/v/net",
        "//src/v/http",
        "//src/v/serde",
        "//src/v/config",
        "//src/v/kafka:types",
    ],
)
