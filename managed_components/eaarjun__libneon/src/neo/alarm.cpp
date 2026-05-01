//
// Created by spak on 4/28/23.
//

#include <esp_log.h>
#include <neo/alarm.hpp>

namespace neo {
    namespace {
        constexpr std::uint32_t ticks_in_ms = timer::resolution_hz / 1'000;
    }

    std::chrono::milliseconds alarm::alarm_elapsed() const {
        if (handle() != nullptr) {
            std::uint64_t ticks = 0;
            ESP_ERROR_CHECK(gptimer_get_raw_count(handle(), &ticks));
            return std::chrono::milliseconds{ticks / ticks_in_ms};
        }
        return 0ms;
    }

    void alarm::task_body(void *user_ctx) {
        if (auto *self = static_cast<alarm *>(user_ctx); self != nullptr) {
            // Wait until the task is signalled a start
            if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) == 0) {
                ESP_LOGE("TIMER", "Unable to take notification on task!");
                return;
            }
            ESP_LOGI("TIMER", "Timer running on core %d.", xPortGetCoreID());
            // Terminate when the task own pointer is set to null
            while (self->_cbk_task != nullptr) {
                if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) != 0) {
                    // Don't call null pointers. Note that this implies that we must
                    // either lock around cbk_fn or never change it.
                    if (self->_cbk_fn != nullptr) {
                        self->_cbk_fn(*self);
                    }
                }
            }
            ESP_LOGI("TIMER", "Timer stopped on core %d.", xPortGetCoreID());
        } else {
            ESP_LOGE("TIMER", "Null timer instance in callback body.");
        }
    }

    bool alarm::unlock_task() {
        // Make a copy atomically so that we do not risk to incur in races
        if (TaskHandle_t task = _cbk_task; task != nullptr) {
            BaseType_t high_task_awoken = pdFALSE;
            vTaskNotifyGiveFromISR(task, &high_task_awoken);
            // Return whether we need to yield at the end of ISR
            return high_task_awoken == pdTRUE;
        }
        return false;
    }

    bool alarm::alarm_body(gptimer_handle_t, const gptimer_alarm_event_data_t *edata, void *user_ctx) {
        if (auto *self = static_cast<alarm *>(user_ctx); self != nullptr) {
            return self->unlock_task();
        }
        return false;
    }


    void alarm::delete_task() {
        // Make a copy atomically so that we do not risk to incur in races
        if (TaskHandle_t task = _cbk_task; task != nullptr) {
            _cbk_task = nullptr;
            _cbk_fn = nullptr;
            // Unlock the scheduler
            unlock_task();
            // In the meanwhile delete the task
            vTaskDelete(task);
        }
    }

    void alarm::set_priority(UBaseType_t priority) {
        vTaskPrioritySet(_cbk_task, priority);
    }

    void alarm::create_task(BaseType_t affinity) {
        assert(not is_active());
        assert(_cbk_task == nullptr);
        TaskHandle_t new_task = nullptr;
        const auto res = xTaskCreatePinnedToCore(
                &task_body,
                "neo::timer",
                CONFIG_ESP_TIMER_TASK_STACK_SIZE,
                this,
                3 | portPRIVILEGE_BIT,
                &new_task,
                affinity);
        if (res != pdPASS) {
            ESP_LOGE("TIMER", "Unable to create pinned timer task, error %d.", res);
            return;
        }
        _core_affinity = affinity;
        _cbk_task = new_task;
        // Unlock it
        unlock_task();
    }

    void alarm::setup_alarm(std::chrono::milliseconds period) {
        assert(handle() != nullptr);
        const gptimer_alarm_config_t alarm_cfg = {
                .alarm_count = static_cast<uint64_t>(period.count()) * ticks_in_ms,
                .reload_count = 0,
                .flags = {.auto_reload_on_alarm = true}};
        ESP_ERROR_CHECK(gptimer_set_alarm_action(handle(), &alarm_cfg));
        _period = period;
    }

    void alarm::setup_callback(std::function<void(alarm &)> callback) {
        assert(_cbk_task != nullptr);
        assert(handle() != nullptr);
        assert(not is_active());
        const gptimer_event_callbacks_t cbk_cfg = {
                .on_alarm = &alarm_body};
        // The change must be performed on a disabled timer
        ESP_ERROR_CHECK(gptimer_disable(handle()));
        ESP_ERROR_CHECK(gptimer_register_event_callbacks(handle(), &cbk_cfg, this));
        ESP_ERROR_CHECK(gptimer_enable(handle()));
        _cbk_fn = std::move(callback);
    }

    void alarm::set_period(std::chrono::milliseconds p) {
        if (handle() != nullptr) {
            setup_alarm(p);
        }
    }

    alarm::alarm(std::chrono::milliseconds period, std::function<void(alarm &)> cbk_fn,
                 BaseType_t affinity) : alarm{} {
        create_task(affinity);
        setup_callback(std::move(cbk_fn));
        setup_alarm(period);
    }

    alarm::~alarm() {
        delete_task();
    }
}// namespace neo