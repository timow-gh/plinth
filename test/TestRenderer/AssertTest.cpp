#include <Core/Assert.hpp>
#include <gtest/gtest.h>

TEST(AssertTest, AssertionWritesFormattedMessageToStderr) {
    testing::internal::CaptureStderr();

    core::assertion("Source.cpp", 42, "run_test", "value != nullptr");

    const std::string output = testing::internal::GetCapturedStderr();
    EXPECT_EQ(output, "Source.cpp:42: internal check failed in 'run_test': 'value != nullptr'\n");
}
