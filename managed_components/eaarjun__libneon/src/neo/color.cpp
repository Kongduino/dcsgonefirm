//
// Created by spak on 6/4/21.
//
#include <string>
#include <cstdio>
#include <neo/color.hpp>

namespace neo {

    srgb_gamma_channel_extractor const &srgb_linear_channel_extractor() {
        static srgb_gamma_channel_extractor _extractor{1.f};
        return _extractor;
    }

    std::string srgb::to_string() const {
        // Do not use stringstream, it requires tons of memory
        std::string buffer;
        buffer.resize(8);
        std::snprintf(buffer.data(), buffer.size(), "#%02x%02x%02x", r, g, b);
        buffer.pop_back();
        return buffer;
    }

    srgb srgb::blend(srgb target, float factor) const {
        factor = std::clamp(factor, 0.f, 1.f);
        return from_linear({to_linear(r) * (1.f - factor) + to_linear(target.r) * factor,
                            to_linear(g) * (1.f - factor) + to_linear(target.g) * factor,
                            to_linear(b) * (1.f - factor) + to_linear(target.b) * factor});
    }

    srgb srgb::lerp(srgb target, float factor) const {
        factor = std::clamp(factor, 0.f, 1.f);
        return {
                std::uint8_t(std::clamp(float(r) * (1.f - factor) + float(target.r) * factor, 0.f, 255.f)),
                std::uint8_t(std::clamp(float(g) * (1.f - factor) + float(target.g) * factor, 0.f, 255.f)),
                std::uint8_t(std::clamp(float(b) * (1.f - factor) + float(target.b) * factor, 0.f, 255.f))};
    }

    hsv srgb::to_hsv() const {
        static constexpr auto hue_scale = 1.f / 6.f;
        const auto [lin_r, lin_g, lin_b] = to_linear(*this);

        const float value = std::max(lin_r, std::max(lin_g, lin_b));
        const float min_c = std::min(lin_r, std::min(lin_g, lin_b));
        const float chroma = value - min_c;

        // Note: explicitly copy structured bindings
        const float hue = [&, lin_r = lin_r, lin_g = lin_g, lin_b = lin_b]() -> float {
            if (chroma < std::numeric_limits<float>::epsilon()) {
                return 0.f;
            } else if (value == lin_r) {
                return hue_scale * float(lin_g - lin_b) / float(chroma);
            } else if (value == lin_g) {
                return hue_scale * (2.f + float(lin_b - lin_r) / float(chroma));
            } else if (value == lin_b) {
                return hue_scale * (4.f + float(lin_r - lin_g) / float(chroma));
            }
            return 0.f;
        }();

        const float saturation = (value < std::numeric_limits<float>::epsilon() ? 0.f : chroma / value);

        return {std::fmod(hue + 1.f, 1.f), saturation, value};
    }

    srgb hsv::to_rgb() const {
        const float s_ = std::clamp(s, 0.f, 1.f);
        const float v_ = std::clamp(v, 0.f, 1.f);
        if (s_ < std::numeric_limits<float>::epsilon()) {
            const std::uint8_t gray = srgb::from_linear(v_);
            return {gray, gray, gray};
        }
        // Remap in 0...6
        const float h_ = 6.f * modclamp(h);
        const auto hue_floor = std::floor(h_);
        const auto hue_block = unsigned(hue_floor) % 6;
        const float m = v_ * (1.f - s_);
        const float x = v_ * (1.f - s_ * (hue_block % 2 == 0 ? 1.f - h_ + hue_floor : h_ - hue_floor));
        switch (hue_block) {
            case 0:
                return srgb::from_linear({v_, x, m});
            case 1:
                return srgb::from_linear({x, v_, m});
            case 2:
                return srgb::from_linear({m, v_, x});
            case 3:
                return srgb::from_linear({m, x, v_});
            case 4:
                return srgb::from_linear({x, m, v_});
            default:
                return srgb::from_linear({v_, m, x});
        }
    }

    hsv hsv::clamped() const {
        return {
                modclamp(h),
                std::clamp(s, 0.f, 1.f),
                std::clamp(v, 0.f, 1.f)};
    }

}// namespace neo
