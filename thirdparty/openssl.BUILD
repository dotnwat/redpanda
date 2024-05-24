load("@rules_foreign_cc//foreign_cc:defs.bzl", "configure_make")

filegroup(
    name = "openssl_srcs",
    srcs = glob(["**"]),
)

configure_make(
    name = "libopenssl",
    lib_source = ":openssl_srcs",
    out_lib_dir = "lib64",
    out_shared_libs = ["libssl.so"],
    configure_command = "Configure",
    configure_options = [
        "enable-fips",
        "--debug",
    ],
    visibility = [
        "//visibility:public",
    ],
)
