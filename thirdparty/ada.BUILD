load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake")

filegroup(
    name = "ada_srcs",
    srcs = glob(["**"]),
)

cmake(
    name = "libada",
    lib_source = ":ada_srcs",
    cache_entries = {
      "BUILD_SHARED_LIBS": "ON",
      "ADA_TESTING": "OFF",
      "ADA_TOOLS": "OFF",
      "ADA_BENCHMARKS": "OFF",
    },
    out_lib_dir = "lib64",
    out_shared_libs = ["libada.so"],
    visibility = [
        "//visibility:public",
    ],
)
