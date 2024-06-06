load("@rules_foreign_cc//foreign_cc:defs.bzl", "configure_make")

filegroup(
    name = "libpciaccess_srcs",
    srcs = glob(["**"]),
)

configure_make(
    name = "libpciaccess",
    lib_source = ":libpciaccess_srcs",
    configure_in_place = True,
    autoreconf = True,
    autoreconf_options = ["-ivf"],
    out_shared_libs = ["libpciaccess.so"],
    out_static_libs = ["libpciaccess.a"],
    configure_options = [
      "--enable-shared",
      "--enable-static",
    ],
    visibility = [
        "//visibility:public",
    ],
)
