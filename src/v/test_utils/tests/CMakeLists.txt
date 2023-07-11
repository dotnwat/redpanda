find_package(GTest REQUIRED)
add_executable(gtest_test gtest_test.cc)
target_link_libraries(gtest_test Seastar::seastar GTest::gtest_main)
