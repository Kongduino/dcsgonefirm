//
// Created by spak on 6/4/21.
//

#ifndef NEO_COLOR_HPP
#define NEO_COLOR_HPP

#include <array>
#include <neo/channel.hpp>
#include <neo/math.hpp>

namespace neo {
    struct srgb;
    struct hsv;

    namespace literals {
        constexpr srgb operator""_rgb(unsigned long long int);
    }

    /**
     * The values are expressed in sRGB color space.
     */
    struct srgb {
        std::uint8_t r = 0;
        std::uint8_t g = 0;
        std::uint8_t b = 0;

        constexpr srgb() = default;

        explicit constexpr srgb(std::uint32_t rgb_);

        constexpr srgb(std::uint8_t r_, std::uint8_t g_, std::uint8_t b_);

        [[nodiscard]] srgb blend(srgb target, float factor) const;
        [[nodiscard]] srgb lerp(srgb target, float factor) const;

        [[nodiscard]] hsv to_hsv() const;

        [[nodiscard]] constexpr static float to_linear(std::uint8_t v);
        [[nodiscard]] constexpr static std::uint8_t from_linear(float v);

        [[nodiscard]] constexpr static std::array<float, 3> to_linear(srgb const &rgb);
        [[nodiscard]] constexpr static srgb from_linear(std::array<float, 3> const &linear_rgb);

        [[nodiscard]] std::string to_string() const;

        [[nodiscard]] constexpr std::uint8_t operator[](channel c) const;
    };

    struct srgb_gamma_channel_extractor {
        std::array<std::uint8_t, 0x100> lut;

        constexpr explicit srgb_gamma_channel_extractor(float gamma = 1.f);

        [[nodiscard]] constexpr std::uint8_t operator()(srgb col, channel chn) const;
    };

    /**
     * @todo Deprecate when it can become constexpr (need constexpr std::pow)
     */
    [[nodiscard]] srgb_gamma_channel_extractor const &srgb_linear_channel_extractor();

    struct hsv {
        float h = 0.f;
        float s = 0.f;
        float v = 0.f;

        constexpr hsv() = default;

        constexpr hsv(float h_, float s_, float v_);

        [[nodiscard]] hsv clamped() const;

        [[nodiscard]] srgb to_rgb() const;
    };

}// namespace neo

namespace neo {

    constexpr float srgb::to_linear(std::uint8_t v) {
        const float f = byte_to_unit(v);
        return f <= 0.04045f ? f / 12.92f : std::pow((f + 0.055f) / 1.055f, 2.4f);
    }

    constexpr std::uint8_t srgb::from_linear(float v) {
        v = v <= 0.0031308f ? v * 12.92f : 1.055f * std::pow(v, 1.f / 2.4f) - 0.055f;
        return unit_to_byte(v);
    }

    constexpr std::array<float, 3> srgb::to_linear(srgb const &rgb) {
        return {to_linear(rgb.r), to_linear(rgb.g), to_linear(rgb.b)};
    }

    constexpr srgb srgb::from_linear(std::array<float, 3> const &linear_rgb) {
        return {from_linear(linear_rgb[0]), from_linear(linear_rgb[1]), from_linear(linear_rgb[2])};
    }

    constexpr srgb_gamma_channel_extractor::srgb_gamma_channel_extractor(float gamma) : lut{} {
        for (std::uint16_t val = 0x00; val <= 0xff; ++val) {
            if (gamma == 1.f) {
                lut[val] = unit_to_byte(srgb::to_linear(std::uint8_t(val)));
            } else {
                lut[val] = unit_to_byte(std::pow(srgb::to_linear(std::uint8_t(val)), gamma));
            }
        }
    }

    constexpr std::uint8_t srgb_gamma_channel_extractor::operator()(srgb col, channel chn) const {
        return lut[col[chn]];
    }

    constexpr srgb literals::operator""_rgb(unsigned long long int c) {
        return srgb{std::uint32_t(c)};
    }

    constexpr srgb::srgb(std::uint8_t r_, std::uint8_t g_, std::uint8_t b_)
        : r{r_}, g{g_}, b{b_} {}

    constexpr srgb::srgb(std::uint32_t rgb_)
        : r{std::uint8_t(0xff & (rgb_ >> 16))},
          g{std::uint8_t(0xff & (rgb_ >> 8))},
          b{std::uint8_t(0xff & rgb_)} {}

    constexpr std::uint8_t srgb::operator[](channel c) const {
        switch (c) {
            case channel::r:
                return r;
            case channel::g:
                return g;
            case channel::b:
                return b;
        }
        return 0;
    }

    constexpr hsv::hsv(float h_, float s_, float v_) : h{h_}, s{s_}, v{v_} {}

}// namespace neo

#endif//NEO_COLOR_HPP
