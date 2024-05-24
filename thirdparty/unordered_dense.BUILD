load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake")

filegroup(
    name = "unordered_dense_srcs",
    srcs = glob(["**"]),
)

cmake(
    name = "libunordered_dense",
    lib_source = ":unordered_dense_srcs",
    out_headers_only = True,
    visibility = [
        "//visibility:public",
    ],
)
