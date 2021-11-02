find_package(Hdrhistogram)
find_package(Base64)

# Base64::base64
# find_package(Base64)
v_cc_library(
  NAME utils
  SRCS
    hdr_hist.cc
    human.cc
    file_io.cc
    base64.cc
    retry_chain_node.cc
  DEPS
    Seastar::seastar
    hdr_histogram_static
    v::rphashing
    v::rprandom
    v::bytes)

#add_subdirectory(tests)
