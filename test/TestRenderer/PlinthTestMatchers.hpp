#pragma once

#include <cmath>
#include <gtest/gtest.h>
#include <tuple>

template <typename ActualVec, typename ExpectedVec>
inline ::testing::AssertionResult Double3Near(const ActualVec& actual, const ExpectedVec& expected, double tolerance) {
    for (int i = 0; i < 3; ++i) {
        const double diff = std::abs(static_cast<double>(actual[i] - expected[i]));
        if (diff > tolerance) {
            return ::testing::AssertionFailure()
                   << "component[" << i << "] expected " << expected[i] << " got " << actual[i] << " (diff " << diff
                   << " > tolerance " << tolerance << ")";
        }
    }

    return ::testing::AssertionSuccess();
}

template <typename Vec3Tuple>
inline ::testing::AssertionResult UnitVec3f(const Vec3Tuple& values, float tolerance) {
    const float x = static_cast<float>(std::get<0>(values));
    const float y = static_cast<float>(std::get<1>(values));
    const float z = static_cast<float>(std::get<2>(values));
    const float length = std::sqrt(x * x + y * y + z * z);

    if (std::abs(length - 1.0f) > tolerance) {
        return ::testing::AssertionFailure() << "length was " << length << " (diff " << std::abs(length - 1.0f)
                                             << " > tolerance " << tolerance << ")";
    }

    return ::testing::AssertionSuccess();
}
