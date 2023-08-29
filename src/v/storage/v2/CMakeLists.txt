v_cc_library(
  NAME storage_v2
  SRCS
    segment.cc
  DEPS
    Seastar::seastar
    absl::btree
)

add_subdirectory(tests)
