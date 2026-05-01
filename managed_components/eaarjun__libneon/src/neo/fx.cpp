//
// Created by spak on 8/19/23.
//

#include <neo/encoder.hpp>
#include <neo/fx.hpp>

namespace neo {
    using namespace literals;

    void solid_fx::populate(alarm const &, color_range colors) {
        std::fill(std::begin(colors), std::end(colors), color);
    }

    void gradient_fx::populate(alarm const &a, color_range colors) {
        const float rotation = rotate_cycle_time > 0ms ? a.cycle_time(rotate_cycle_time) : 0.f;
        gradient_sample(std::begin(gradient), std::end(gradient), colors.size(), std::begin(colors), rotation, scale);
    }

    void pulse_fx::populate(const neo::alarm &a, color_range colors) {
        _buffer.clear();
        _buffer.resize(colors.size());
        // Make it black so that we know what the state is
        std::fill(std::begin(colors), std::end(colors), srgb{});

        color_range rg{_buffer};

        if (lo) {
            lo->populate(a, rg);
        }
        if (hi) {
            hi->populate(a, colors);
        }

        float t = cycle_time > 0ms ? a.cycle_time(cycle_time) : 0.f;

        // Cycle is really half of it
        t = 1.f - 2.f * std::abs(t - 0.5f);

        broadcast_blend(std::begin(rg), std::end(rg),
                        std::begin(colors), std::end(colors),
                        std::begin(colors), t);
    }

    bool transition_fx::transition::is_complete(std::chrono::milliseconds t) const {
        return activation_time + transition_duration < t;
    }

    float transition_fx::transition::compute_blend_factor(std::chrono::milliseconds t) const {
        return std::clamp(float(t.count() - activation_time.count()) / float(transition_duration.count()), 0.f, 1.f);
    }

    void transition_fx::pop_expired(std::chrono::milliseconds t) {
        while (_active_transitions.size() > 1 and _active_transitions[1].is_complete(t)) {
            // If the next transition is complete, the predecessor is not needed
            _active_transitions.pop_front();
        }
    }

    void transition_fx::populate(const neo::alarm &a, color_range colors) {
        std::fill(std::begin(colors), std::end(colors), 0x0_rgb);

        pop_expired(a.total_elapsed());
        _buffer.clear();
        _buffer.resize(colors.size());

        color_range rg{_buffer};

        for (transition const &item : _active_transitions) {
            if (item.is_complete(a.total_elapsed())) {
                // Just take the final result
                item.fx->populate(a, colors);
                continue;
            }
            // Blend the old colors with the new
            const float blend_factor = item.compute_blend_factor(a.total_elapsed());
            item.fx->populate(a, rg);
            broadcast_blend(std::begin(colors), std::end(colors),
                            std::begin(rg), std::end(rg),
                            std::begin(colors), blend_factor);
        }
    }

    void transition_fx::transition_to(alarm const &a, std::shared_ptr<fx_base> fx, std::chrono::milliseconds duration) {
        _active_transitions.emplace_back(transition{a.total_elapsed(), duration, std::move(fx)});
    }


    std::function<void(alarm &)> fx_base::make_callback(led_encoder &encoder, std::size_t num_leds) {
        return [fx = shared_from_this(), buffer = std::vector<neo::srgb>{num_leds}, enc = &encoder](neo::alarm &a) mutable {
            fx->populate(a, buffer);
            ESP_ERROR_CHECK(enc->transmit(std::begin(buffer), std::end(buffer), neo::srgb_linear_channel_extractor()));
        };
    }

    void blend_fx::populate(const neo::alarm &a, color_range colors) {
        _buffer.clear();
        _buffer.resize(colors.size());
        // Make it black so that we know what the state is
        std::fill(std::begin(colors), std::end(colors), srgb{});

        color_range rg{_buffer};

        if (lo) {
            lo->populate(a, rg);
        }
        if (hi) {
            hi->populate(a, colors);
        }

        broadcast_blend(std::begin(rg), std::end(rg),
                        std::begin(colors), std::end(colors),
                        std::begin(colors), blend_factor);
    }

}// namespace neo