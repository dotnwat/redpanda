load("@rules_foreign_cc//foreign_cc:defs.bzl", "configure_make")

filegroup(
    name = "krb5_srcs",
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
    name = "krb5",
    lib_source = ":krb5_srcs",
    configure_in_place = True,
    autoreconf = True,
    autoreconf_options = ["-ivf ./src"],
    out_shared_libs = ["libkrb5.so"],
    configure_command = "./src/configure",
    configure_options = [
        "--srcdir=./src",
        "--disable-thread-support",
        "--without-netlib",
        "--enable-shared",
        "--disable-static",
    ],
    visibility = [
        "//visibility:public",
    ],
)
