#include "test_utils/test.h"

#include <gtest/gtest.h>

namespace {
std::vector<std::string> ss_args;
std::vector<char*> ss_argv;
std::once_flag seastar_runner_init_flag;
void init_seastar_test_runner() {
    ASSERT_TRUE(seastar::testing::global_test_runner().start(
      ss_argv.size(), ss_argv.data()));
}
} // namespace

void seastar_test::SetUpTestSuite() {
    std::call_once(seastar_runner_init_flag, init_seastar_test_runner);
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);

    for (int i = 0; i < argc; ++i) {
        ss_args.push_back(argv[i]);
        ss_argv.push_back(ss_args.back().data());
    }

    int ret = RUN_ALL_TESTS();

    int ss_ret = seastar::testing::global_test_runner().finalize();
    if (ret) {
        return ret;
    }
    return ss_ret;
}
