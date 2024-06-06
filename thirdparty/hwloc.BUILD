load("@rules_foreign_cc//foreign_cc:defs.bzl", "configure_make")

filegroup(
    name = "hwloc_srcs",
    srcs = glob(["**"]),
)

configure_make(
    name = "hwloc",
    lib_source = ":hwloc_srcs",
    configure_in_place = True,
    autoreconf = True,
    autoreconf_options = ["-ivf"],
    out_shared_libs = ["libhwloc.so"],
    out_static_libs = ["libhwloc.a"],
    configure_options = [
      "--disable-libudev",
      "--enable-shared",
      "--enable-static",
    ],
    visibility = [
        "//visibility:public",
    ],
)
