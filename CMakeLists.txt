# Redirect control for internal Redpanda builds
if(VECTORIZED_CMAKE_DIR)
  cmake_minimum_required(VERSION 3.22)
  list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
  include(${VECTORIZED_CMAKE_DIR}/main.cmake)
  return()
endif()

cmake_minimum_required(VERSION 3.24)
project(redpanda LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# make "lld" be the default since a full statically linked build using "ld" will
# often fail due to being oom killed at the end of the build when many large
# targets are being linked in parallel.
include(CheckLinkerFlag)
set(Redpanda_LINKER "lld" CACHE STRING "Linker to use")
set(Redpanda_LINKER_FLAGS "-fuse-ld=${Redpanda_LINKER}")
check_linker_flag(CXX ${Redpanda_LINKER_FLAGS} HAVE_Redpanda_LINKER_FLAGS)
if(NOT HAVE_Redpanda_LINKER_FLAGS)
  message(FATAL_ERROR "Linker ${Redpanda_LINKER} not found or not supported")
endif()
add_link_options(${Redpanda_LINKER_FLAGS})

# enable sanitizers. will propogate to all targets, including dependencies
option(Redpanda_ENABLE_SANITIZERS "Enable sanitizers (address, leak, undefined)" OFF)
if(Redpanda_ENABLE_SANITIZERS)
  add_link_options(-fsanitize=address,leak,undefined)
  add_compile_options(-fsanitize=address,leak,undefined)
endif()

# We use enums as bitflags in seastar and Redpanda, which
# creating a constexpr enum value that is bitwise combination
# of flags can trigger this warning as the resulting "enum",
# isn't a valid value.
add_compile_options(-Wno-enum-constexpr-conversion)


find_package(absl CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(ada CONFIG REQUIRED)
find_package(Boost CONFIG REQUIRED
  COMPONENTS filesystem iostreams unit_test_framework)
find_package(Crc32c CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)
find_package(LibXml2 CONFIG REQUIRED)
if (RP_OPENSSL_VERSION)
  find_package(OpenSSL CONFIG ${RP_OPENSSL_VERSION} EXACT REQUIRED)
else()
  find_package(OpenSSL CONFIG 3.0.0 REQUIRED)
endif()
find_package(Roaring CONFIG REQUIRED)
find_package(Snappy CONFIG REQUIRED)
find_package(zstd CONFIG REQUIRED)
find_package(cryptopp CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)
find_package(RapidJSON CONFIG REQUIRED)
find_package(re2 CONFIG REQUIRED)
find_package(xxHash CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(lz4 CONFIG REQUIRED)
find_package(Avro CONFIG REQUIRED)
find_package(Base64 CONFIG REQUIRED)
find_package(Avro CONFIG REQUIRED)
find_package(hdr_histogram CONFIG REQUIRED)

find_package(ZLIB REQUIRED)

#find_package(GnuTLS REQUIRED)
# find_package(c-ares CONFIG REQUIRED)
# TODO gmp pkg-config
# TODO hwloc pkg-config

include(dependencies)

find_package(Seastar CONFIG REQUIRED)
find_package(unordered_dense REQUIRED)
find_program(GO_PROGRAM go REQUIRED)
find_program(SEASTAR_JSON2CODE seastar-json2code.py REQUIRED)

function (seastar_generate_swagger)
  set (one_value_args TARGET VAR IN_FILE OUT_DIR)
  cmake_parse_arguments (args "" "${one_value_args}" "" ${ARGN})
  get_filename_component (in_file_name ${args_IN_FILE} NAME)
  set (generator ${SEASTAR_JSON2CODE})
  set (header_out ${args_OUT_DIR}/${in_file_name}.hh)
  set (source_out ${args_OUT_DIR}/${in_file_name}.cc)

  add_custom_command (
    DEPENDS
      ${args_IN_FILE}
      ${generator}
    OUTPUT ${header_out} ${source_out}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${args_OUT_DIR}
    COMMAND ${generator} --create-cc -f ${args_IN_FILE} -o ${header_out})

  add_custom_target (${args_TARGET}
    DEPENDS
      ${header_out}
      ${source_out})

  set (${args_VAR} ${header_out} ${source_out} PARENT_SCOPE)
endfunction ()

# normalize the name of the gnutls dependency. when seastar is imported via a
# normal installation arrange for the library name given by seastar's find
# module to match the name from the system cmake find module for gnutls.
#if (TARGET GnuTLS::gnutls)
#  add_library(GnuTLS::GnuTLS ALIAS GnuTLS::gnutls)
#else()
#find_package(GnuTLS REQUIRED)
#endif()


include(v_library)
include(testing)

option(Redpanda_ENABLE_COVERAGE "Enable coverage" OFF)
if(Redpanda_ENABLE_COVERAGE)
  add_compile_options(-O0 -g --coverage)
  add_link_options(--coverage)
endif()

#
# Configure clang-tidy checks. By default checks will be enabled if the
# clang-tidy tool is found. Otherwise, it can be explicitly disabled. It's
# recommended to use FORCE_ON in contexts such as CI where we want to fail
# configuration if clang-tidy is not available.
#
set(Redpanda_ENABLE_CLANG_TIDY "ON" CACHE STRING
  "Enable clang-tidy checks if available. Can be ON, OFF, or FORCE_ON")

if(NOT Redpanda_ENABLE_CLANG_TIDY STREQUAL OFF)
  if(Redpanda_ENABLE_CLANG_TIDY STREQUAL FORCE_ON)
    find_program(CLANG_TIDY_COMMAND clang-tidy REQUIRED)
  else()
    find_program(CLANG_TIDY_COMMAND clang-tidy)
  endif()
  if(CLANG_TIDY_COMMAND)
    set(Redpanda_ENABLE_CLANG_TIDY ON)
  else()
    set(Redpanda_ENABLE_CLANG_TIDY OFF)
  endif()
endif()

add_subdirectory(src)
