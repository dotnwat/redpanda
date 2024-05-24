load("@rules_foreign_cc//foreign_cc:defs.bzl", "configure_make")

filegroup(
    name = "gmp_srcs",
    srcs = glob(["**"]),
)

# out_lib_dir = "lib64",
configure_make(
    name = "libgmp",
    lib_source = ":gmp_srcs",
    configure_in_place = True,
    autoreconf = True,
    autoreconf_options = ["-ivf"],
    out_shared_libs = ["libgmp.so"],
    out_static_libs = ["libgmp.a"],
    configure_options = [
      "--enable-shared",
      "--enable-static",
    ],
    visibility = [
        "//visibility:public",
    ],
)
