cc_library(
    name = "compression",
    srcs = [
        "compression.cc",
        "stream_zstd.cc",
        "async_stream_zstd.cc",
        "snappy_standard_compressor.cc",
        "lz4_decompression_buffers.cc",
        "internal/snappy_java_compressor.cc",
        "internal/lz4_frame_compressor.cc",
        "internal/gzip_compressor.cc",
    ],
    hdrs = [
        "include/compression/async_stream_zstd.h",
        "include/compression/compression.h",
        "include/compression/lz4_decompression_buffers.h",
        "include/compression/snappy_standard_compressor.h",
        "include/compression/stream_zstd.h",
        "include/compression/internal/gzip_compressor.h",
        "include/compression/internal/lz4_frame_compressor.h",
        "include/compression/internal/snappy_java_compressor.h",
        "include/compression/internal/zstd_compressor.h",
    ],
    includes = [
        "include",
    ],
    copts = [
        "-DZSTD_STATIC_LINKING_ONLY"
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@seastar//:libseastar",
        "@snappy//:libsnappy",
        "//src/v/base",
        "//src/v/bytes",
        "//src/v/model",
        "//src/v/utils",
        "@zstd",
        "@zlib",
        "@lz4",
    ],
)
