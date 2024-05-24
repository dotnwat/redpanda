load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake")

filegroup(
    name = "wasmtime_srcs",
    srcs = glob(["**"]),
)

cmake(
    name = "libwasmtime",
    lib_source = ":wasmtime_srcs",
    working_directory = "crates/c-api",
    cache_entries = {
        "WASMTIME_USER_CARGO_BUILD_OPTIONS": "--no-default-features;--features=async;--features=addr2line;--features=wat",
    },
    out_static_libs = ["libwasmtime.a"],
    visibility = [
        "//visibility:public",
    ],
)
