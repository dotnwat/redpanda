#include "gtest/gtest.h"
#include "test_utils/test.h"

#include <seastar/core/sleep.hh>

// set argv/argc from main and then use if needed in these test suites
class seastar_test2 : public seastar_test {};

TEST_F(seastar_test, Foo) {
    run([]() -> seastar::future<> {
        co_await seastar::sleep(std::chrono::seconds(1));
    });
}

TEST_F(seastar_test, Foo2) {
    run([]() -> seastar::future<> {
        co_await seastar::sleep(std::chrono::seconds(1));
    });
}

TEST_F(seastar_test2, Foo3) {
    run([]() -> seastar::future<> {
        co_await seastar::sleep(std::chrono::seconds(1));
    });
}

struct Asdf : public testing::Test {};

TEST_F(Asdf, Foo3) {}

TEST(Llkjlkj, Aasdfs) {}

SS_TEST(Hello, GoodByte) { co_await seastar::sleep(std::chrono::seconds(1)); }

class BarTest
  : public seastar_test
  , public testing::WithParamInterface<int> {};

SS_TEST_P(BarTest, Aasdsss) {
    co_await seastar::sleep(std::chrono::seconds(1));
}

INSTANTIATE_TEST_SUITE_P(Sup, BarTest, testing::Values(1, 2, 3));

SS_TEST_F(seastar_test2, asdf) {
    co_await seastar::sleep(std::chrono::seconds(1));
}
