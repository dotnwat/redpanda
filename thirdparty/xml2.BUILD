load("@rules_foreign_cc//foreign_cc:defs.bzl", "configure_make")

filegroup(
    name = "xml2_srcs",
    srcs = glob(["**"]),
)

configure_make(
    name = "libxml2",
    lib_source = ":xml2_srcs",
    env = {"NOCONFIGURE": "true"},
    configure_in_place = True,
    autogen = True,
    autoreconf = True,
    autoreconf_options = ["-ivf"],
    out_shared_libs = ["libxml2.so"],
    out_static_libs = ["libxml2.a"],
    configure_options = [
      "--without-python",
      "--enable-shared",
      "--enable-static",
    ],
    out_include_dir = "include/libxml2",
    deps = [
        "@zlib",
        "@xz//:libxz",
    ],
    visibility = [
        "//visibility:public",
    ],
)
