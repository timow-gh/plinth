#include <OpenGL/ErrorReporting.hpp>
#include <gtest/gtest.h>
#include <string>
#include <vector>

TEST(ErrorReportingTest, CustomSinkReceivesReportedMessages) {
    std::vector<std::string> received;
    opengl::set_error_sink([&received](opengl::ErrorSeverity severity, std::string_view message) {
        received.emplace_back(severity == opengl::ErrorSeverity::error ? "error: " : "warning: ");
        received.back() += message;
    });

    opengl::report_error("x");

    ASSERT_EQ(received.size(), 1U);
    EXPECT_EQ(received.front(), "error: x");

    opengl::set_error_sink(nullptr);
}
