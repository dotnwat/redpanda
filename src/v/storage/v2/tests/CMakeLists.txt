rp_test(
  UNIT_TEST
  GTEST
  BINARY_NAME segment
  SOURCES
    segment_test.cc
  LIBRARIES
    v::gtest_main
    v::storage_v2
)
