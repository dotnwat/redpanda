load("@rules_foreign_cc//foreign_cc:defs.bzl", "configure_make")

filegroup(
    name = "xz_srcs",
    srcs = glob(["**"]),
)

configure_make(
    name = "libxz",
    lib_source = ":xz_srcs",
    configure_in_place = True,
    autoreconf = True,
    autoreconf_options = ["-ivf"],
    out_shared_libs = ["liblzma.so"],
    out_static_libs = ["liblzma.a"],
    configure_options = [
      "--enable-shared",
      "--enable-static",
    ],
    visibility = [
        "//visibility:public",
    ],
)
