v_cc_library(
  NAME container
  DEPS
    roaring::roaring
    unordered_dense::unordered_dense
)

add_subdirectory(tests)
