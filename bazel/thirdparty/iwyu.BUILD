load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "libclang_imported",
    srcs = ["@current_llvm_toolchain_llvm//:libclang"],
    hdrs = ["@current_llvm_toolchain_llvm//:all_includes"],
    visibility = ["//visibility:public"],
)

cc_binary(
    name = "iwyu",
    srcs = [
        "iwyu.cc",
        "iwyu_ast_util.cc",
        "iwyu_ast_util.h",
        "iwyu_cache.cc",
        "iwyu_cache.h",
        "iwyu_driver.cc",
        "iwyu_driver.h",
        "iwyu_getopt.cc",
        "iwyu_getopt.h",
        "iwyu_globals.cc",
        "iwyu_globals.h",
        "iwyu_include_picker.cc",
        "iwyu_include_picker.h",
        "iwyu_lexer_utils.cc",
        "iwyu_lexer_utils.h",
        "iwyu_location_util.cc",
        "iwyu_location_util.h",
        "iwyu_output.cc",
        "iwyu_output.h",
        "iwyu_path_util.cc",
        "iwyu_path_util.h",
        "iwyu_port.cc",
        "iwyu_port.h",
        "iwyu_preprocessor.cc",
        "iwyu_preprocessor.h",
        "iwyu_regex.cc",
        "iwyu_regex.h",
        "iwyu_stl_util.h",
        "iwyu_string_util.h",
        "iwyu_use_flags.h",
        "iwyu_verrs.cc",
        "iwyu_verrs.h",
        "iwyu_version.h",
    ],
    copts = [
        "-Iexternal/toolchains_llvm++llvm+current_llvm_toolchain_llvm/include",
        "-Wno-deprecated-declarations",
        "-Wno-deprecated-this-capture",
        "-Wno-deprecated-anon-enum-enum-conversion",
        "-Wno-vexing-parse",
    ],
    defines = [
        "IWYU_GIT_REV=\\\"0000000\\\"",

        # FIXME
        "IWYU_RESOURCE_BINARY_PATH=\\\"TBD\\\"",
        "IWYU_RESOURCE_DIR=\\\"TBD\\\"",
    ],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":libclang_imported",
    ],
)
