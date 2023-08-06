#pragma once

#include <seastar/core/app-template.hh>
#include <seastar/core/coroutine.hh>
#include <seastar/core/seastar.hh>
#include <seastar/testing/test_runner.hh>
#include <seastar/util/log.hh>

#include <gtest/gtest.h>

class seastar_test : public ::testing::Test {
protected:
    static void SetUpTestSuite();

public:
    void run(std::function<seastar::future<>()> task) {
        seastar::testing::global_test_runner().run_sync(std::move(task));
    }
};

// Helper macro for defining tests.
#define SS_GTEST_TEST_(test_suite_name, test_name, parent_class, parent_id)    \
    static_assert(                                                             \
      sizeof(GTEST_STRINGIFY_(test_suite_name)) > 1,                           \
      "test_suite_name must not be empty");                                    \
    static_assert(                                                             \
      sizeof(GTEST_STRINGIFY_(test_name)) > 1, "test_name must not be empty"); \
    class GTEST_TEST_CLASS_NAME_(test_suite_name, test_name)                   \
      : public parent_class {                                                  \
    public:                                                                    \
        GTEST_TEST_CLASS_NAME_(test_suite_name, test_name)() = default;        \
        ~GTEST_TEST_CLASS_NAME_(test_suite_name, test_name)() override         \
          = default;                                                           \
        GTEST_TEST_CLASS_NAME_(test_suite_name, test_name)                     \
        (const GTEST_TEST_CLASS_NAME_(test_suite_name, test_name) &) = delete; \
        GTEST_TEST_CLASS_NAME_(test_suite_name, test_name) &                   \
        operator=(const GTEST_TEST_CLASS_NAME_(test_suite_name, test_name) &)  \
          = delete; /* NOLINT */                                               \
        GTEST_TEST_CLASS_NAME_(test_suite_name, test_name)                     \
        (GTEST_TEST_CLASS_NAME_(test_suite_name, test_name) &&) noexcept       \
          = delete;                                                            \
        GTEST_TEST_CLASS_NAME_(test_suite_name, test_name) & operator=(        \
          GTEST_TEST_CLASS_NAME_(test_suite_name, test_name) &&) noexcept      \
          = delete; /* NOLINT */                                               \
                                                                               \
    private:                                                                   \
        void TestBody() override {                                             \
            run([this] { return TestBodyWrapped(); });                         \
        }                                                                      \
        seastar::future<> TestBodyWrapped();                                   \
        static ::testing::TestInfo* const test_info_ GTEST_ATTRIBUTE_UNUSED_;  \
    };                                                                         \
                                                                               \
    ::testing::TestInfo* const GTEST_TEST_CLASS_NAME_(                         \
      test_suite_name, test_name)::test_info_                                  \
      = ::testing::internal::MakeAndRegisterTestInfo(                          \
        #test_suite_name,                                                      \
        #test_name,                                                            \
        nullptr,                                                               \
        nullptr,                                                               \
        ::testing::internal::CodeLocation(__FILE__, __LINE__),                 \
        (parent_id),                                                           \
        ::testing::internal::SuiteApiResolver<                                 \
          parent_class>::GetSetUpCaseOrSuite(__FILE__, __LINE__),              \
        ::testing::internal::SuiteApiResolver<                                 \
          parent_class>::GetTearDownCaseOrSuite(__FILE__, __LINE__),           \
        new ::testing::internal::TestFactoryImpl<GTEST_TEST_CLASS_NAME_(       \
          test_suite_name, test_name)>);                                       \
    seastar::future<> GTEST_TEST_CLASS_NAME_(                                  \
      test_suite_name, test_name)::TestBodyWrapped()

#define SS_TEST(test_suite_name, test_name)                                    \
    SS_GTEST_TEST_(                                                            \
      test_suite_name,                                                         \
      test_name,                                                               \
      seastar_test,                                                            \
      ::testing::internal::GetTestTypeId())

#define SS_TEST_F(test_fixture, test_name)                                     \
    SS_GTEST_TEST_(                                                            \
      test_fixture,                                                            \
      test_name,                                                               \
      test_fixture,                                                            \
      ::testing::internal::GetTypeId<test_fixture>())

#define SS_TEST_P(test_suite_name, test_name)                                  \
    class GTEST_TEST_CLASS_NAME_(test_suite_name, test_name)                   \
      : public test_suite_name {                                               \
    public:                                                                    \
        GTEST_TEST_CLASS_NAME_(test_suite_name, test_name)() {}                \
        void TestBody() override {                                             \
            run([this] { return TestBodyWrapped(); });                         \
        }                                                                      \
        seastar::future<> TestBodyWrapped();                                   \
                                                                               \
    private:                                                                   \
        static int AddToRegistry() {                                           \
            ::testing::UnitTest::GetInstance()                                 \
              ->parameterized_test_registry()                                  \
              .GetTestSuitePatternHolder<test_suite_name>(                     \
                GTEST_STRINGIFY_(test_suite_name),                             \
                ::testing::internal::CodeLocation(__FILE__, __LINE__))         \
              ->AddTestPattern(                                                \
                GTEST_STRINGIFY_(test_suite_name),                             \
                GTEST_STRINGIFY_(test_name),                                   \
                new ::testing::internal::TestMetaFactory<                      \
                  GTEST_TEST_CLASS_NAME_(test_suite_name, test_name)>(),       \
                ::testing::internal::CodeLocation(__FILE__, __LINE__));        \
            return 0;                                                          \
        }                                                                      \
        static int gtest_registering_dummy_ GTEST_ATTRIBUTE_UNUSED_;           \
        GTEST_DISALLOW_COPY_AND_ASSIGN_(                                       \
          GTEST_TEST_CLASS_NAME_(test_suite_name, test_name));                 \
    };                                                                         \
    int GTEST_TEST_CLASS_NAME_(                                                \
      test_suite_name, test_name)::gtest_registering_dummy_                    \
      = GTEST_TEST_CLASS_NAME_(test_suite_name, test_name)::AddToRegistry();   \
    seastar::future<> GTEST_TEST_CLASS_NAME_(                                  \
      test_suite_name, test_name)::TestBodyWrapped()
