include(FetchContent)

set(FETCHCONTENT_QUIET FALSE)

# don't require that cache entries be created for crc32c options so we can use
# normal variables via set(). a better solution here would be to go update the
# crc32c cmake build.
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

function(fetch_dep NAME)
  cmake_parse_arguments(fetch_dep_args "" "REPO;TAG" "" ${ARGN})
  FetchContent_Declare(
    ${NAME}
    GIT_REPOSITORY ${fetch_dep_args_REPO}
    GIT_TAG ${fetch_dep_args_TAG}
    GIT_SHALLOW ON
    GIT_SUBMODULES ""
    GIT_PROGRESS TRUE
    USES_TERMINAL_DOWNLOAD TRUE
    OVERRIDE_FIND_PACKAGE
    SYSTEM
    ${fetch_dep_args_UNPARSED_ARGUMENTS})
endfunction()

fetch_dep(fmt
  REPO https://github.com/fmtlib/fmt.git
  TAG 8.1.1)

# CMakeLists.txt is patched to avoid registering tests. We still want the
# Seastar testing library to be built, but we don't want the tests to run. This
# could be accomplished with Seastar_INSTALL=ON, but this doesn't play nice with
# the add_subdirectory method of using Seastar.
set(Seastar_TESTING ON)
fetch_dep(seastar
  REPO https://github.com/redpanda-data/seastar.git
  TAG 2fb425e034c453ceff736d6107d187ebe7fdb9c4
  PATCH_COMMAND sed -i "s/add_subdirectory (tests/# add_subdirectory (tests/g" CMakeLists.txt)

fetch_dep(avro
  REPO https://github.com/dotnwat/avro
  TAG abadfb649e69a5a85adba1e236a873f7a8a9690a
  SOURCE_SUBDIR redpanda_build)

fetch_dep(rapidjson
  REPO https://github.com/dotnwat/rapidjson.git
  TAG 14a5dd756e9bef26f9b53d3b4eb1b73c6a1794d5
  SOURCE_SUBDIR redpanda_build)

set(CRC32C_BUILD_TESTS OFF)
set(CRC32C_BUILD_BENCHMARKS OFF)
set(CRC32C_USE_GLOG OFF)
set(CRC32C_INSTALL OFF)
fetch_dep(crc32c
  REPO https://github.com/google/crc32c.git
  TAG 1.1.2)

set(BASE64_BUILD_CLI OFF)
set(BASE64_BUILD_TESTS OFF)
fetch_dep(base64
  REPO https://github.com/aklomp/base64.git
  TAG v0.5.0)

fetch_dep(roaring
  REPO https://github.com/dotnwat/CRoaring.git
  TAG 1ee85130b2283ca0a968258ea171ec0af69b69d8
  SOURCE_SUBDIR redpanda_build)

fetch_dep(GTest
  REPO https://github.com/google/googletest
  TAG f8d7d77c06936315286eb55f8de22cd23c188571)

fetch_dep(hdrhistogram
  REPO https://github.com/HdrHistogram/HdrHistogram_c
  TAG 0.11.5)

FetchContent_MakeAvailable(
    fmt
    rapidjson
    seastar
    GTest
    crc32c
    base64
    roaring
    avro
    hdrhistogram)

add_library(Crc32c::crc32c ALIAS crc32c)
add_library(aklomp::base64 ALIAS base64)
add_library(Hdrhistogram::hdr_histogram ALIAS hdr_histogram)
