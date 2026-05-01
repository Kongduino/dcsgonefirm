//
// Created by spak on 5/2/23.
//

#ifndef LIBNEON_MATH_HPP
#define LIBNEON_MATH_HPP

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace neo {
    [[nodiscard]] constexpr float modclamp(float f, float low = 0.f, float high = 1.f);

    [[nodiscard]] constexpr std::uint8_t unit_to_byte(float f);
    [[nodiscard]] constexpr float byte_to_unit(std::uint8_t b);
}// namespace neo

namespace neo {

    constexpr float modclamp(float f, float low, float high) {
        f = (f - low) / (high - low);
        return std::lerp(low, high, f - std::floor(f));
    }

    constexpr std::uint8_t unit_to_byte(float f) {
        return std::uint8_t(std::round(std::clamp(f * 255.f, 0.f, 255.f)));
    }

    constexpr float byte_to_unit(std::uint8_t b) {
        return float(b) / 255.f;
    }

}// namespace neo

#endif//LIBNEON_MATH_HPP
