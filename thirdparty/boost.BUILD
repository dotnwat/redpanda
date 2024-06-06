load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake")

filegroup(
    name = "boost_srcs",
    srcs = glob(["**"]),
)

cmake(
    name = "boost",
    lib_source = ":boost_srcs",
    cache_entries = {
        "BUILD_SHARED_LIBS": "ON",
        "BOOST_INCLUDE_LIBRARIES": "system;filesystem;thread;program_options;thread;test",
        "BOOST_RUNTIME_LINK": "shared",
        "BOOST_INSTALL_LAYOUT": "system",
        "BOOST_LOCALE_ENABLE_ICU": "OFF",
    },
    out_lib_dir = "lib64",
    out_shared_libs = [
        "libboost_atomic.so",
        "libboost_chrono.so",
        "libboost_container.so",
        "libboost_date_time.so",
        "libboost_filesystem.so",
        "libboost_program_options.so",
        "libboost_thread.so",
        "libboost_prg_exec_monitor.so",
        "libboost_unit_test_framework.so",
    ],
    visibility = [
        "//visibility:public",
    ],
)

#cc_library(
#    name = "boost",
#    srcs = glob([
#        "libs/serialization/src/**/*.cpp",
#        "libs/serialization/src/**/*.ipp",
#    ]),
#    hdrs = glob([
#        "boost/**/*.h",
#        "boost/**/*.hpp",
#        "boost/**/*.ipp",
#    ]),
#    includes = [
#        "."
#    ],
#    visibility = ["//visibility:public"],
#)
