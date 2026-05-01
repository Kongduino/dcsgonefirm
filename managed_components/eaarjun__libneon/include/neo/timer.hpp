//
// Created by spak on 6/7/21.
//

#ifndef NEO_TIMER_HPP
#define NEO_TIMER_HPP

#include <atomic>
#include <chrono>
#include <driver/gptimer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <functional>
#include <memory>


namespace neo {

    namespace {
        using namespace std::chrono_literals;
    }

    class timer {
        gptimer_handle_t _hdl;
        bool _active;
        std::chrono::milliseconds _prev_laps_duration;
        std::chrono::time_point<std::chrono::steady_clock> _last_start;

    protected:
        [[nodiscard]] inline gptimer_handle_t handle() const;

    public:
        static constexpr std::uint32_t resolution_hz = 1'000'000;

        timer();

        timer(timer const &) = delete;
        timer &operator=(timer const &) = delete;

        timer(timer &&) noexcept = delete;
        timer &operator=(timer &&) noexcept = delete;

        void start();
        void stop();
        void reset();

        [[nodiscard]] inline bool is_active() const;

        [[nodiscard]] std::chrono::milliseconds lap_elapsed() const;
        [[nodiscard]] std::chrono::milliseconds total_elapsed() const;

        [[nodiscard]] float cycle_time(std::chrono::milliseconds wanted_period, std::chrono::milliseconds offset = 0ms) const;

        ~timer();
    };

}// namespace neo

namespace neo {

    bool timer::is_active() const {
        return _active;
    }

    gptimer_handle_t timer::handle() const {
        return _hdl;
    }

}// namespace neo

#endif//NEO_TIMER_HPP
