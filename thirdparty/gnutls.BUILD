load("@rules_foreign_cc//foreign_cc:defs.bzl", "configure_make")

filegroup(
    name = "gnutls_srcs",
    srcs = glob(["**"]),
)

#sed -i s/-Wa,-march=all//g lib/accelerated/aarch64/Makefile.am
configure_make(
    name = "libgnutls",
    lib_source = ":gnutls_srcs",
    configure_in_place = True,
    autoreconf = True,
    autoreconf_options = ["-ivf"],
    out_shared_libs = ["libgnutls.so"],
    out_static_libs = ["libgnutls.a"],
    env = {"GTKDOCIZE": "echo"},
    configure_options = [
      "--with-included-unistring",
      "--with-included-libtasn1",
      "--without-idn",
      "--without-brotli",
      "--without-p11-kit",
      # https://lists.gnupg.org/pipermail/gnutls-help/2016-February/004085.html
      "--disable-non-suiteb-curves",
      "--disable-doc",
      "--disable-tests",
      "--enable-shared",
      "--enable-static",
    ],
    deps = [
        "@nettle//:libnettle",
    ],
    visibility = [
        "//visibility:public",
    ],
)
