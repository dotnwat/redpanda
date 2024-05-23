load("@rules_foreign_cc//foreign_cc:defs.bzl", "configure_make")

filegroup(
    name = "numactl_srcs",
    srcs = glob(["**"]),
)

configure_make(
    name = "libnumactl",
    lib_source = ":numactl_srcs",
    configure_in_place = True,
    autoreconf = True,
    autoreconf_options = ["-ivf"],
    out_shared_libs = ["libnuma.so"],
    out_static_libs = ["libnuma.a"],
    configure_options = [
      "CFLAGS='-Dredacted=\"redacted\"'",
      "--enable-shared",
      "--enable-static",
    ],
    visibility = [
        "//visibility:public",
    ],
)
