v_cc_library(
  NAME seastar_testing_main
  SRCS seastar_testing_main.cc
  DEPS Seastar::seastar_testing v::rprandom)

v_cc_library(
  NAME gtest_mainx
  SRCS gtest_main.cc
  DEPS Seastar::seastar Seastar::seastar_testing GTest::gtest)

add_subdirectory(tests)
