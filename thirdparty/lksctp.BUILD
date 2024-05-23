load("@rules_foreign_cc//foreign_cc:defs.bzl", "configure_make")

filegroup(
    name = "lksctp_srcs",
    srcs = glob(["**"]),
)

configure_make(
    name = "liblksctp",
    lib_source = ":lksctp_srcs",
    configure_in_place = True,
    autoreconf = True,
    autoreconf_options = ["-ivf"],
    out_shared_libs = ["libsctp.so"],
    out_static_libs = ["libsctp.a"],
    configure_options = [
        "--enable-shared",
        "--enable-static",
    ],
    visibility = [
        "//visibility:public",
    ],
)
