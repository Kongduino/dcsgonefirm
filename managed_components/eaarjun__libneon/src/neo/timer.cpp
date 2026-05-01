//
// Created by spak on 6/7/21.
//

#include <cmath>
#include <esp_log.h>
#include <neo/math.hpp>
#include <neo/timer.hpp>


namespace neo {

    namespace {
        constexpr gptimer_config_t default_gptimer_config{
                .clk_src = GPTIMER_CLK_SRC_DEFAULT,
                .direction = GPTIMER_COUNT_UP,
                .resolution_hz = timer::resolution_hz,
                .flags = {.intr_shared = false}};
    }// namespace

    void timer::start() {
        if (not _active and _hdl != nullptr) {
            ESP_ERROR_CHECK(gptimer_start(_hdl));
            _active = true;
            _last_start = std::chrono::steady_clock::now();
        }
    }

    void timer::stop() {
        if (_active and _hdl != nullptr) {
            ESP_ERROR_CHECK(gptimer_stop(_hdl));
            // Get it before stopping
            _prev_laps_duration += lap_elapsed();
            _active = false;
        }
    }

    void timer::reset() {
        if (_hdl != nullptr) {
            ESP_ERROR_CHECK(gptimer_set_raw_count(_hdl, 0));
            _prev_laps_duration = 0ms;
            if (_active) {
                _last_start = std::chrono::steady_clock::now();
            }
        }
    }

    std::chrono::milliseconds timer::lap_elapsed() const {
        if (not _active) {
            return 0ms;
        }
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - _last_start);
    }

    std::chrono::milliseconds timer::total_elapsed() const {
        return _prev_laps_duration + lap_elapsed();
    }

    float timer::cycle_time(std::chrono::milliseconds wanted_period, std::chrono::milliseconds offset) const {
        return modclamp(float((total_elapsed() + offset).count()) / float(wanted_period.count()));
    }

    timer::timer()
        : _hdl{nullptr},
          _active{false},
          _prev_laps_duration{0ms},
          _last_start{std::numeric_limits<std::chrono::time_point<std::chrono::steady_clock>>::max()} {
        ESP_ERROR_CHECK(gptimer_new_timer(&default_gptimer_config, &_hdl));
        ESP_ERROR_CHECK(gptimer_enable(_hdl));
    }

    timer::~timer() {
        stop();
        ESP_ERROR_CHECK(gptimer_disable(_hdl));
        ESP_ERROR_CHECK(gptimer_del_timer(_hdl));
        _hdl = nullptr;
    }


}// namespace neo