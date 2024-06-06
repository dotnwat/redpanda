load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake")

filegroup(
    name = "base64_srcs",
    srcs = glob(["**"]),
)

#if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64")
#  list(APPEND BASE64_CMAKE_ARGS
#    -DBASE64_WITH_SSSE3=ON
#    -DBASE64_WITH_SSE41=ON
#    -DBASE64_WITH_SSE42=ON
#    -DBASE64_WITH_AVX=OFF
#    -DBASE64_WITH_AVX2=OFF
#    -DBASE64_WITH_NEON32=OFF
#    -DBASE64_WITH_NEON64=OFF
#  )
#elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64")
#  list(APPEND BASE64_CMAKE_ARGS
#    -DBASE64_WITH_SSSE3=OFF
#    -DBASE64_WITH_SSE41=OFF
#    -DBASE64_WITH_SSE42=OFF
#    -DBASE64_WITH_AVX=OFF
#    -DBASE64_WITH_AVX2=OFF
#    -DBASE64_WITH_NEON32=OFF
#    -DBASE64_WITH_NEON32=OFF
#    -DBASE64_WITH_NEON64=ON
#  )
#endif()

cmake(
    name = "libbase64",
    lib_source = ":base64_srcs",
    cache_entries = {
        "BASE64_WITH_OpenMP": "OFF",
        "BASE64_WERROR": "OFF",
    },
    out_lib_dir = "lib64",
    out_static_libs = ["libbase64.a"],
    visibility = [
        "//visibility:public",
    ],
)
