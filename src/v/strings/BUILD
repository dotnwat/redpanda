cc_library(
    name = "strings",
    srcs = [
        "utf8.cc",
    ],
    hdrs = [
        "include/strings/string_switch.h",
        "include/strings/utf8.h",
    ],
    includes = [
        "include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//src/v/base",
    ],
)

cc_test(
    name = "strings_test",
    srcs = [
        "tests/constexpr_string_switch.cc",
        "tests/string_switch_test.cc",
        "tests/utf8_control_chars.cc",
    ],
    deps = [
        "//src/v/strings",
        "@boost",
    ],
)
