load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake")

filegroup(
    name = "cryptopp_srcs",
    srcs = glob(["**"]),
)

cmake(
    name = "libcryptopp",
    lib_source = ":cryptopp_srcs",
    cache_entries = {
        "BUILD_SHARED": "ON",
        "BUILD_TESTING": "OFF",
    },
    out_lib_dir = "lib64",
    out_shared_libs = ["libcryptopp.so"],
    visibility = [
        "//visibility:public",
    ],
)
