#pragma once

#include <cmath>

namespace domain {

inline constexpr float kEpsilon = 1e-5f;

inline float lengthSq(float x, float y) {
    return x * x + y * y;
}

inline float length(float x, float y) {
    return std::sqrt(lengthSq(x, y));
}

/** In-place normalize; if near zero, sets (1,0). */
inline void normalizeOrDefault(float& x, float& y) {
    const float L = length(x, y);
    if (L > kEpsilon) {
        x /= L;
        y /= L;
    } else {
        x = 1.f;
        y = 0.f;
    }
}

} // namespace domain
