load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake")

filegroup(
    name = "roaring_srcs",
    srcs = glob(["**"]),
)

cmake(
    name = "libroaring",
    lib_source = ":roaring_srcs",
    cache_entries = {
      "BUILD_SHARED_LIBS": "ON",
      "ENABLE_ROARING_TESTS": "OFF",
      "ROARING_DISABLE_NEON": "ON", # we target -march=westmere which doesn't have avx
      "ROARING_DISABLE_AVX": "ON", # we target -march=westmere which doesn't have avx
      "ROARING_DISABLE_NATIVE": "ON",
    },
    out_shared_libs = ["libroaring.so"],
    visibility = [
        "//visibility:public",
    ],
)
