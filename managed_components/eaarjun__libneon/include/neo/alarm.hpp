//
// Created by spak on 4/28/23.
//

#ifndef LIBNEON_ALARM_HPP
#define LIBNEON_ALARM_HPP

#include <neo/timer.hpp>

namespace neo {

    namespace literals {
        using namespace std::chrono_literals;

        constexpr std::chrono::milliseconds operator""_fps(unsigned long long int fps);
    }// namespace literals

    class alarm : public timer {
        std::atomic<TaskHandle_t> _cbk_task = nullptr;
        std::function<void(alarm &)> _cbk_fn = nullptr;
        BaseType_t _core_affinity = tskNO_AFFINITY;
        std::chrono::milliseconds _period = std::numeric_limits<std::chrono::milliseconds>::max();

        static void task_body(void *user_ctx);
        static bool alarm_body(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx);

        /**
         * @return True if the task has already awaken.
         */
        bool unlock_task();

        /**
         * - The task must have not been set up before.
         * - The timer must not be active.
         */
        void create_task(BaseType_t affinity);

        /**
         * - The timer must have been set up.
         */
        void setup_alarm(std::chrono::milliseconds period);

        /**
         * - The timer must have been set up
         * - The task must have been set up
         * - The timer must not be active
         */
        void setup_callback(std::function<void(alarm &)> callback);

        void delete_task();

        alarm() = default;

    public:
        alarm(std::chrono::milliseconds period, std::function<void(alarm &)> cbk_fn,
              BaseType_t affinity = tskNO_AFFINITY);

        [[nodiscard]] inline BaseType_t core_affinity() const;
        [[nodiscard]] inline std::chrono::milliseconds period() const;

        [[nodiscard]] inline std::function<void(alarm &)> const &callback() const;
        [[nodiscard]] inline std::function<void(alarm &)> &callback();

        void set_period(std::chrono::milliseconds p);

        void set_priority(UBaseType_t priority);

        [[nodiscard]] std::chrono::milliseconds alarm_elapsed() const;

        ~alarm();
    };
}// namespace neo

namespace neo {

    namespace literals {
        constexpr std::chrono::milliseconds operator""_fps(unsigned long long int fps) {
            return 1'000ms / fps;
        }
    }// namespace literals

    BaseType_t alarm::core_affinity() const {
        return _core_affinity;
    }

    std::chrono::milliseconds alarm::period() const {
        return _period;
    }

    std::function<void(alarm &)> const &alarm::callback() const {
        return _cbk_fn;
    }

    std::function<void(alarm &)> &alarm::callback() {
        return _cbk_fn;
    }

}// namespace neo

#endif//LIBNEON_ALARM_HPP
