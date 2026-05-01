//
// Created by spak on 8/19/23.
//

#ifndef LIBNEON_FX_HPP
#define LIBNEON_FX_HPP

#include <deque>
#include <neo/alarm.hpp>
#include <neo/color.hpp>
#include <neo/gradient.hpp>
#include <ranges>
#include <vector>

namespace neo {

    class led_encoder;

    using color_range = std::ranges::subrange<std::vector<srgb>::iterator>;

    struct fx_base : public std::enable_shared_from_this<fx_base> {
        virtual void populate(alarm const &a, color_range colors) = 0;

        [[nodiscard]] std::function<void(alarm &)> make_callback(led_encoder &encoder, std::size_t num_leds);

        template <class Extractor>
        [[nodiscard]] std::function<void(alarm &)> make_callback(led_encoder &encoder, std::size_t num_leds, Extractor extractor);

        virtual ~fx_base() = default;
    };

    struct solid_fx : fx_base {
        srgb color = {};

        solid_fx() = default;
        inline explicit solid_fx(srgb color_);


        void populate(alarm const &, color_range colors) override;
    };

    struct gradient_fx : fx_base {
        std::vector<gradient_entry> gradient = {};
        std::chrono::milliseconds rotate_cycle_time = 0ms;
        float scale = 1.f;

        gradient_fx() = default;
        inline explicit gradient_fx(std::vector<gradient_entry> gradient_, std::chrono::milliseconds rotate_cycle_time_ = 2s, float scale_ = 1.f);
        inline explicit gradient_fx(std::vector<srgb> gradient_, std::chrono::milliseconds rotate_cycle_time_ = 2s, float scale_ = 1.f);

        void populate(alarm const &a, color_range colors) override;
    };

    template <class>
    struct is_fx_ptr : std::false_type {};
    template <class T>
        requires std::is_base_of_v<fx_base, T>
    struct is_fx_ptr<std::shared_ptr<T>> : std::true_type {};

    template <class T>
    concept fx_or_fx_ptr = std::is_base_of_v<fx_base, std::remove_cvref_t<T>> or is_fx_ptr<std::remove_cvref_t<T>>::value;

    template <fx_or_fx_ptr T>
    [[nodiscard]] std::shared_ptr<fx_base> wrap(T &&fx);


    struct pulse_fx : fx_base {
        std::shared_ptr<fx_base> lo = {};
        std::shared_ptr<fx_base> hi = {};
        std::chrono::milliseconds cycle_time = 0ms;

        pulse_fx() = default;

        template <fx_or_fx_ptr Fx1, fx_or_fx_ptr Fx2>
        pulse_fx(Fx1 lo_, Fx2 hi_, std::chrono::milliseconds cycle_time_ = 2s);

        void populate(alarm const &a, color_range colors) override;

    private:
        std::vector<srgb> _buffer;
    };

    class transition_fx : public fx_base {
        struct transition {
            std::chrono::milliseconds activation_time;
            std::chrono::milliseconds transition_duration;
            std::shared_ptr<fx_base> fx;

            [[nodiscard]] bool is_complete(std::chrono::milliseconds t) const;
            [[nodiscard]] float compute_blend_factor(std::chrono::milliseconds t) const;
        };

        std::deque<transition> _active_transitions;
        std::vector<srgb> _buffer;

        void pop_expired(std::chrono::milliseconds t);

    public:
        transition_fx() = default;
        void populate(alarm const &a, color_range colors) override;

        void transition_to(alarm const &a, std::shared_ptr<fx_base> fx, std::chrono::milliseconds duration);

        template <fx_or_fx_ptr Fx>
        void transition_to(alarm const &a, Fx fx, std::chrono::milliseconds duration);
    };

    struct blend_fx : fx_base {
        std::shared_ptr<fx_base> lo = {};
        std::shared_ptr<fx_base> hi = {};
        float blend_factor = 0.5f;

        blend_fx() = default;

        template <fx_or_fx_ptr Fx1, fx_or_fx_ptr Fx2>
        blend_fx(Fx1 lo_, Fx2 hi_, float blend_factor_ = 0.5f);

        void populate(alarm const &a, color_range colors) override;

    private:
        std::vector<srgb> _buffer;
    };

}// namespace neo

namespace neo {
    gradient_fx::gradient_fx(std::vector<gradient_entry> gradient_, std::chrono::milliseconds rotate_cycle_time_, float scale_)
        : gradient{std::move(gradient_)},
          rotate_cycle_time{rotate_cycle_time_},
          scale{scale_} {}

    gradient_fx::gradient_fx(std::vector<srgb> gradient_, std::chrono::milliseconds rotate_cycle_time_, float scale_)
        : gradient{neo::gradient_make_uniform_from_colors(std::move(gradient_))},
          rotate_cycle_time{rotate_cycle_time_},
          scale{scale_} {}

    solid_fx::solid_fx(neo::srgb color_) : color{color_} {}

    template <fx_or_fx_ptr Fx1, fx_or_fx_ptr Fx2>
    pulse_fx::pulse_fx(Fx1 lo_, Fx2 hi_, std::chrono::milliseconds cycle_time_)
        : lo{wrap(std::move(lo_))}, hi{wrap(std::move(hi_))}, cycle_time{cycle_time_} {}


    template <fx_or_fx_ptr T>
    [[nodiscard]] std::shared_ptr<fx_base> wrap(T &&fx) {
        if constexpr (std::is_base_of_v<fx_base, std::remove_cvref_t<T>>) {
            return std::static_pointer_cast<fx_base>(std::make_shared<std::remove_cvref_t<T>>(std::forward<T>(fx)));
        } else {
            return std::static_pointer_cast<fx_base>(std::forward<T>(fx));
        }
    }

    template <fx_or_fx_ptr Fx1, fx_or_fx_ptr Fx2>
    blend_fx::blend_fx(Fx1 lo_, Fx2 hi_, float blend_factor_)
        : lo{wrap(std::move(lo_))}, hi{wrap(std::move(hi_))}, blend_factor{blend_factor_} {}


    template <fx_or_fx_ptr Fx>
    void transition_fx::transition_to(alarm const &a, Fx fx, std::chrono::milliseconds duration) {
        transition_to(a, neo::wrap(std::move(fx)), duration);
    }

    template <class Extractor>
    std::function<void(alarm &)> fx_base::make_callback(led_encoder &encoder, std::size_t num_leds, Extractor extractor) {
        return [fx = shared_from_this(), buffer = std::vector<neo::srgb>{num_leds}, enc = &encoder, extractor = extractor](neo::alarm &a) mutable {
            fx->populate(a, buffer);
            ESP_ERROR_CHECK(enc->transmit(std::begin(buffer), std::end(buffer), extractor));
        };
    }
}// namespace neo

#endif//LIBNEON_FX_HPP
