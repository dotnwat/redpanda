load("@bazel_skylib//rules:native_binary.bzl", "native_binary")

native_binary(
    name = "minio",
    src = select({
        "@platforms//cpu:x86_64": "@minio_x86_64//file",
        "@platforms//cpu:aarch64": "@minio_aarch64//file",
    }),
    visibility = ["//visibility:public"],
)

alias(
    name = "prometheus",
    actual = select({
        "@platforms//cpu:x86_64": "@prometheus_x86_64//:prometheus",
        "@platforms//cpu:aarch64": "@prometheus_aarch64//:prometheus",
    }),
    visibility = ["//visibility:public"],
)

alias(
    name = "grafana",
    actual = select({
        "@platforms//cpu:x86_64": "@grafana_x86_64//:grafana",
        "@platforms//cpu:aarch64": "@grafana_aarch64//:grafana",
    }),
    visibility = ["//visibility:public"],
)

alias(
    name = "otelcol_contrib",
    actual = select({
        "@platforms//cpu:x86_64": "@otelcol_contrib_x86_64//:otelcol_contrib",
        "@platforms//cpu:aarch64": "@otelcol_contrib_aarch64//:otelcol_contrib",
    }),
    visibility = ["//visibility:public"],
)

alias(
    name = "tempo",
    actual = select({
        "@platforms//cpu:x86_64": "@tempo_x86_64//:tempo",
        "@platforms//cpu:aarch64": "@tempo_aarch64//:tempo",
    }),
    visibility = ["//visibility:public"],
)

alias(
    name = "omb_minimal",
    actual = "@omb//:omb",
    visibility = ["//visibility:public"],
)
