#include <array>
#include <thread>
#include <vector>

#include <esp_log.h>

#include <neo/alarm.hpp>
#include <neo/encoder.hpp>
#include <neo/fx.hpp>
#include <neo/gradient.hpp>

using namespace std::chrono_literals;
using namespace neo::literals;

// ─── Simple Blink ────────────────────────────────────────────────────────────

void example_simple_blink(gpio_num_t gpio_pin, std::size_t num_leds) {
    neo::led_encoder encoder{neo::encoding::ws2812b, neo::make_rmt_config(gpio_pin)};
    auto animate = [&, i = std::size_t{0}, buffer = std::vector<neo::srgb>(num_leds, 0x0_rgb)](neo::alarm &a) mutable {
        buffer[i++ % num_leds] = 0x0_rgb;
        buffer[i % num_leds] = 0xaaaaaa_rgb;
        ESP_ERROR_CHECK(encoder.transmit(std::begin(buffer), std::end(buffer)));
    };
    neo::alarm alarm{10_fps, animate};
    alarm.start();
    vTaskSuspend(nullptr);
}

// ─── Static Gradient ─────────────────────────────────────────────────────────

void example_static_gradient(gpio_num_t gpio_pin, std::size_t num_leds) {
    neo::led_encoder encoder{neo::encoding::ws2812b, neo::make_rmt_config(gpio_pin)};
    const auto rainbow = neo::gradient_make_uniform_from_colors(
            {0xff0000_rgb, 0xffff00_rgb, 0x00ff00_rgb, 0x00ffff_rgb, 0x0000ff_rgb, 0xff00ff_rgb, 0xff0000_rgb});
    std::vector<neo::srgb> colors(num_leds);
    neo::gradient_sample(std::begin(rainbow), std::end(rainbow), colors.size(), std::begin(colors));
    ESP_ERROR_CHECK(encoder.transmit(std::begin(colors), std::end(colors), neo::srgb_linear_channel_extractor()));
    vTaskSuspend(nullptr);
}

// ─── Gradient FX (Rainbow) ───────────────────────────────────────────────────

void example_gradient_fx(gpio_num_t gpio_pin, std::size_t num_leds) {
    neo::led_encoder encoder{neo::encoding::ws2812b, neo::make_rmt_config(gpio_pin)};
    const auto rainbow_fx = neo::wrap(neo::gradient_fx{
            {0xff0000_rgb, 0xffff00_rgb, 0x00ff00_rgb, 0x00ffff_rgb, 0x0000ff_rgb, 0xff00ff_rgb, 0xff0000_rgb},
            5s});
    neo::alarm alarm{30_fps, rainbow_fx->make_callback(encoder, num_leds)};
    alarm.start();
    vTaskSuspend(nullptr);
}

// ─── Spinner FX ──────────────────────────────────────────────────────────────

void example_spinner_fx(gpio_num_t gpio_pin, std::size_t num_leds) {
    neo::led_encoder encoder{neo::encoding::ws2812b, neo::make_rmt_config(gpio_pin)};
    const auto spinner_fx = neo::wrap(neo::gradient_fx{
            {{0.0, 0x0_rgb}, {0.1, 0x9999999_rgb}, {0.2, 0x444444_rgb}, {0.9, 0x0_rgb}},
            5s, 2.});
    neo::alarm alarm{30_fps, spinner_fx->make_callback(encoder, num_leds)};
    alarm.start();
    vTaskSuspend(nullptr);
}

// ─── Pulse FX ────────────────────────────────────────────────────────────────

void example_pulse_fx(gpio_num_t gpio_pin, std::size_t num_leds) {
    neo::led_encoder encoder{neo::encoding::ws2812b, neo::make_rmt_config(gpio_pin)};
    const auto pulse_fx = neo::wrap(neo::pulse_fx{
            neo::solid_fx{0x0_rgb},
            neo::solid_fx{0x7fc0c2_rgb},
            4s});
    neo::alarm alarm{30_fps, pulse_fx->make_callback(encoder, num_leds)};
    alarm.start();
    vTaskSuspend(nullptr);
}

// ─── Composite (Cycling FX with Transitions) ─────────────────────────────────

void example_composite(gpio_num_t gpio_pin, std::size_t num_leds) {
    neo::led_encoder encoder{neo::encoding::ws2812b, neo::make_rmt_config(gpio_pin)};
    const std::array<std::shared_ptr<neo::fx_base>, 4> all_fx = {
            // Rainbow:
            neo::wrap(neo::gradient_fx{
                    {0xff0000_rgb, 0xffff00_rgb, 0x00ff00_rgb, 0x00ffff_rgb, 0x0000ff_rgb, 0xff00ff_rgb, 0xff0000_rgb},
                    5s}),
            // Spinner:
            neo::wrap(neo::gradient_fx{
                    {{0.0, 0x0_rgb}, {0.1, 0x9999999_rgb}, {0.2, 0x444444_rgb}, {0.9, 0x0_rgb}},
                    5s, 2.}),
            // Pulse red-yellow:
            neo::wrap(neo::pulse_fx{neo::solid_fx{0xff0000_rgb}, neo::solid_fx{0xffff00_rgb}, 2s}),
            // Pulse blue:
            neo::wrap(neo::pulse_fx{neo::solid_fx{0x0000ff_rgb}, neo::solid_fx{0x000000_rgb}, 2s})};

    auto fx_transition = std::make_shared<neo::transition_fx>();
    auto fx_mask = neo::wrap(neo::blend_fx{fx_transition, neo::solid_fx{0x0_rgb}, 0.75f});

    neo::alarm alarm{30_fps, fx_mask->make_callback(encoder, num_leds)};
    alarm.start();
    std::this_thread::sleep_for(1s);

    for (std::size_t i = 0; true; i = (i + 1) % all_fx.size()) {
        ESP_LOGI("NEO", "Switching to fx no. %d", i);
        fx_transition->transition_to(alarm, all_fx[i], 2s);
        std::this_thread::sleep_for(10s);
    }
}

// ─── Run All ─────────────────────────────────────────────────────────────────

void examples_run_all(gpio_num_t gpio_pin, std::size_t num_leds) {
    example_composite(gpio_pin, num_leds);
}
