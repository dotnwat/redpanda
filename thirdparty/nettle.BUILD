load("@rules_foreign_cc//foreign_cc:defs.bzl", "configure_make")

filegroup(
    name = "nettle_srcs",
    srcs = glob(["**"]),
)

# <SOURCE_DIR>/.bootstrap && sed -i s/ASM_SYMBOL_PREFIX='_'/ASM_SYMBOL_PREFIX=''/g configure.ac
# set(nettle_build_env
#   ${base_env}
#   CFLAGS=${c_flags}\ -I@REDPANDA_DEPS_INSTALL_DIR@/include
#   LDFLAGS=${ld_flags}
# )
# deps: gmp
configure_make(
    name = "libnettle",
    lib_source = ":nettle_srcs",
    configure_in_place = True,
    autogen = True,
    autogen_command = ".bootstrap",
    out_shared_libs = ["libnettle.so", "libhogweed.so"],
    out_static_libs = ["libnettle.a", "libhogweed.a"],
    out_lib_dir = "lib64",
    configure_options = [
        "--disable-documentation",
        "--enable-shared",
        "--enable-static",
        "--enable-x86-aesni",
    ],
    deps = [
        "@gmp//:libgmp",
    ],
    visibility = [
        "//visibility:public",
    ],
)
