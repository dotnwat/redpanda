load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake")

filegroup(
    name = "hdrhistogram_srcs",
    srcs = glob(["**"]),
)

cmake(
    name = "libhdrhistogram",
    lib_source = ":hdrhistogram_srcs",
    cache_entries = {
        "HDR_HISTOGRAM_BUILD_PROGRAMS": "OFF",
        "HDR_HISTOGRAM_BUILD_SHARED": "ON",
        "HDR_HISTOGRAM_BUILD_STATIC": "ON",
    },
    deps = [
        "@zlib",
    ],
    out_lib_dir = "lib64",
    out_shared_libs = ["libhdr_histogram.so"],
    visibility = [
        "//visibility:public",
    ],
)
