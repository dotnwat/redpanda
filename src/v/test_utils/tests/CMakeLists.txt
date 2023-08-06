add_executable(gtest_test gtest_test.cc)
target_link_libraries(gtest_test Seastar::seastar Seastar::seastar_testing
    v::gtest_mainx)
