cmake_minimum_required(VERSION 3.18.0)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcoroutines")

Set(FETCHCONTENT_QUIET FALSE)

include(FetchContent)


#set(Seastar_CXX_FLAGS=${seastar_cxx_flags}
#set(DSeastar_LD_FLAGS=${seastar_ld_flags}"
#set(Seastar_DPDK_MACHINE=wsm
#set(Boost_USE_STATIC_LIBS OFF CACHE BOOL "")
#set(Boost_NO_BOOST_CMAKE ON CACHE BOOL "")
#set(Boost_NO_SYSTEM_PATHS TRUE CACHE BOOL "")
#set(Protobuf_USE_STATIC_LIBS ON CACHE BOOL "")
set(Seastar_INSTALL ON CACHE BOOL "")
set(Seastar_DPDK OFF CACHE BOOL "")
set(Seastar_APPS ON CACHE BOOL "")
set(Seastar_DEMOS OFF CACHE BOOL "")
set(Seastar_DOCS OFF CACHE BOOL "")
set(Seastar_TESTING OFF CACHE BOOL "")
set(Seastar_API_LEVEL 6)
set(Seastar_CXX_DIALECT c++20)
set(Seastar_UNUSED_RESULT_ERROR ON CACHE BOOL "")

FetchContent_Declare(
  seastar
  GIT_REPOSITORY https://github.com/vectorizedio/seastar.git
  GIT_TAG d8313208857d8362d68b769ec03f338bbcbf482d
)

set(BUILD_TESTING OFF)
FetchContent_Declare(
  abseil
  URL https://storage.googleapis.com/vectorizedio-public/dependencies/abseil-cpp-20210324.1.tar.gz
  URL_MD5 f4d5342d294d1a397a46d6057901e66c
)

set(HDR_HISTOGRAM_BUILD_PROGRAMS OFF)
set(HDR_HISTOGRAM_BUILD_SHARED OFF)
FetchContent_Declare(
  hdr_histogram
  URL https://storage.googleapis.com/vectorizedio-public/dependencies/HdrHistogram_c-0.11.2.tar.gz
  URL_MD5 95970dea64f1a7a8d199aeb9f7c15e60
)

set(CRC32C_BUILD_TESTS OFF CACHE BOOL "")
set(CRC32C_BUILD_BENCHMARKS OFF CACHE BOOL "")
set(CRC32C_USE_GLOG OFF CACHE BOOL "")
set(CRC32C_INSTALL OFF CACHE BOOL "")
FetchContent_Declare(
  crc32c
  URL https://storage.googleapis.com/vectorizedio-public/dependencies/crc32c-1.1.1-PR-47.tar.gz
  URL_MD5 a4fd05e928a44ed8ae61a6a8b92935f9
)

set(ENABLE_ROARING_TESTS OFF CACHE BOOL "")
set(ROARING_BUILD_STATIC ON CACHE BOOL "")
set(ROARING_DISABLE_NEON ON CACHE BOOL "") # we target -march=westmere which doesn't have avx
set(ROARING_DISABLE_AVX ON CACHE BOOL "")# we target -march=westmere which doesn't have avx
set(ROARING_DISABLE_NATIVE ON)
FetchContent_Declare(
  roaring
  GIT_REPOSITORY https://github.com/RoaringBitmap/CRoaring.git
  GIT_TAG 9809c8e60a5ca8f9175fe30831a9047eb2e60861
)

FetchContent_MakeAvailable(seastar abseil hdr_histogram crc32c roaring)

add_subdirectory(src)

## keep this minimal. modify `cmake/main.cmake` file instead
#if(NOT VECTORIZED_CMAKE_DIR)
#  include(main)
#else()
#  include(${VECTORIZED_CMAKE_DIR}/main.cmake)
#endif()
