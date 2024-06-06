#load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake")
#
#filegroup(
#    name = "avro_srcs",
#    srcs = glob(["**"]),
#)
#
#cmake(
#    name = "libavro",
#    lib_source = ":avro_srcs",
#    working_directory = "lang/c++",
#    cache_entries = {
#      "CMAKE_SKIP_INSTALL_ALL_DEPENDENCY": "ON",
#      "BUILD_SHARED_LIBS": "ON",
#    },
#    targets = ["avrocpp", "avrocpp_s", "avrogencpp"],
#    out_shared_libs = ["libavro.so"],
#    deps = [
#        "@boost//:boost",
#    ],
#    visibility = [
#        "//visibility:public",
#    ],
#)

cc_library(
    name = "libavro",
    srcs = glob([
      "lang/c++/impl/*.cc",
      "lang/c++/impl/json/*.cc",
      "lang/c++/impl/json/*.hh",
      "lang/c++/impl/parsing/*.cc",
      "lang/c++/impl/parsing/*.hh",
    ]),
    defines = ["AVRO_VERSION=1"],
    #strip_include_prefix = "lang/c++/include/avro/",
    includes = [
        "lang/c++/include/avro",
        "lang/c++/include",
    ],
    hdrs = glob([
        "lang/c++/include/avro/*.hh",
        "lang/c++/include/avro/buffer/*.hh",
        "lang/c++/include/avro/buffer/detail/*.hh",
    ]),
    deps = [
        "@boost//:boost",
    ],
    visibility = [
        "//visibility:public",
    ],
)


#find_package(Snappy)
#find_package (Boost 1.74 REQUIRED
#  COMPONENTS iostreams system)
#if(SNAPPY_FOUND)
#  target_compile_definitions(avro PUBLIC SNAPPY_CODEC_AVAILABLE)
#endif()
#target_link_libraries(avro
#    Boost::iostreams
#    Boost::system)
#
#target_include_directories(avro
#  PUBLIC ../lang/c++/include Boost::boost
#  PRIVATE ../lang/c++/include/avro)
#
#add_library(Avro::avro ALIAS avro)
