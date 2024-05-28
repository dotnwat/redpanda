load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake")

filegroup(
    name = "snappy_srcs",
    srcs = glob(["**"]),
)

cmake(
    name = "libsnappy",
    lib_source = ":snappy_srcs",
    cache_entries = {
      "BUILD_SHARED_LIBS": "ON",
      "SNAPPY_INSTALL": "ON",
      "SNAPPY_BUILD_TESTS": "OFF",
      "SNAPPY_BUILD_BENCHMARKS": "OFF",
      "CMAKE_SHARED_LINKER_FLAGS": "-Wno-fuse-ld-path",
    },
    out_lib_dir = "lib64",
    out_shared_libs = ["libsnappy.so"],
    visibility = [
        "//visibility:public",
    ],
)
