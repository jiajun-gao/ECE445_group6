#include "ir_manager.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace robot {

static constexpr const char* TAG = "ir_mgr";

bool IrManager::Init() {
    if (initialized_) return true;

    gpio_config_t cfg = {};
    cfg.pin_bit_mask = (1ULL << IR_FRONT_GPIO) | (1ULL << IR_BOTTOM_GPIO);
    cfg.mode         = GPIO_MODE_INPUT;
    // Weak pull-up: if a sensor is disconnected the line floats HIGH ==
    //   front: no obstacle (safe)
    //   bottom: no floor -> cliff (fails safe to STOP)
    cfg.pull_up_en   = GPIO_PULLUP_ENABLE;
    cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    cfg.intr_type    = GPIO_INTR_DISABLE;
    if (gpio_config(&cfg) != ESP_OK) {
        ESP_LOGE(TAG, "gpio_config failed");
        return false;
    }

    initialized_ = true;
    ESP_LOGI(TAG, "IR DO inputs ready (front GPIO%d active=%d, bottom GPIO%d active=%d)",
             IR_FRONT_GPIO, FRONT_ACTIVE_LEVEL, IR_BOTTOM_GPIO, BOTTOM_ACTIVE_LEVEL);
    return true;
}

void IrManager::TaskTrampoline(void* arg) {
    static_cast<IrManager*>(arg)->TaskLoop();
}

void IrManager::TaskLoop() {
    // Per-sensor debounce: keep N-sample streak of identical raw level before
    // flipping the stable state.
    bool     front_stable = false;   // no obstacle initially
    bool     bottom_stable = true;   // assume floor present initially
    uint8_t  f_streak = 0, b_streak = 0;
    int      f_last = -1, b_last = -1;

    // FreeRTOS tick is 100 Hz by default on ESP-IDF, so the minimum
    // vTaskDelayUntil period is 10 ms (1 tick). 100 Hz sampling with 5-sample
    // debounce = 50 ms to accept a transition, which is still well under any
    // meaningful desk-edge response time.
    const TickType_t period = pdMS_TO_TICKS(10);
    TickType_t last = xTaskGetTickCount();

    // Periodic raw-reading dump. Helps distinguish "sensor not wired" from
    // "polarity inverted" from "pot tuned wrong" during bring-up.
    uint32_t dbg_ticks = 0;
    constexpr uint32_t kDebugPeriodTicks = 200;  // 200 * 10ms = 2 s

    // Cumulative "has this pin ever gone active-low" counters. If a channel
    // never trips during the first few seconds of monitoring, the module is
    // almost certainly unwired or its comparator pot is maxed out.
    uint32_t f_low_count = 0, b_low_count = 0;
    uint32_t sample_count = 0;

    while (true) {
        int f_lvl = gpio_get_level(IR_FRONT_GPIO);
        int b_lvl = gpio_get_level(IR_BOTTOM_GPIO);
        if (f_lvl == 0) ++f_low_count;
        if (b_lvl == 0) ++b_low_count;
        ++sample_count;
        bool f_raw_active = (f_lvl == FRONT_ACTIVE_LEVEL);   // obstacle?
        bool b_raw_active = (b_lvl == BOTTOM_ACTIVE_LEVEL);  // floor present?

        if (f_lvl == f_last) {
            if (f_streak < IR_DEBOUNCE_HITS) ++f_streak;
            if (f_streak >= IR_DEBOUNCE_HITS) front_stable = f_raw_active;
        } else { f_streak = 1; f_last = f_lvl; }

        if (b_lvl == b_last) {
            if (b_streak < IR_DEBOUNCE_HITS) ++b_streak;
            if (b_streak >= IR_DEBOUNCE_HITS) bottom_stable = b_raw_active;
        } else { b_streak = 1; b_last = b_lvl; }

        front_obstacle_.store(front_stable, std::memory_order_relaxed);
        bottom_floor_.store(bottom_stable, std::memory_order_relaxed);

        // Cliff has priority: losing floor is more dangerous than an obstacle.
        SafetyReason c = SafetyReason::OK;
        if (!bottom_stable)       c = SafetyReason::CLIFF_DETECTED;
        else if (front_stable)    c = SafetyReason::FRONT_OBSTACLE;
        classification_.store(static_cast<uint8_t>(c), std::memory_order_relaxed);

        if (++dbg_ticks >= kDebugPeriodTicks) {
            dbg_ticks = 0;
            const char* reason =
                c == SafetyReason::OK              ? "OK" :
                c == SafetyReason::FRONT_OBSTACLE  ? "FRONT_OBSTACLE" :
                                                     "CLIFF_DETECTED";
            ESP_LOGI(TAG,
                     "raw f=%d b=%d | lows f=%lu/%lu b=%lu/%lu -> "
                     "front_obstacle=%d bottom_floor=%d reason=%s",
                     f_lvl, b_lvl,
                     (unsigned long)f_low_count, (unsigned long)sample_count,
                     (unsigned long)b_low_count, (unsigned long)sample_count,
                     front_stable ? 1 : 0, bottom_stable ? 1 : 0, reason);

            // One-time warning: if after ~4 seconds of sampling a channel has
            // never once read LOW, the wiring or pot is almost certainly
            // wrong. Only fire once so it stays visible in the log.
            static bool warned_front = false, warned_bottom = false;
            if (sample_count > 400 && f_low_count == 0 && !warned_front) {
                warned_front = true;
                ESP_LOGW(TAG,
                         "FRONT sensor (GPIO%d) never read LOW after %lu samples. "
                         "Check DOUT wiring, 3V3/GND, or turn the module pot.",
                         IR_FRONT_GPIO, (unsigned long)sample_count);
            }
            if (sample_count > 400 && b_low_count == 0 && !warned_bottom) {
                warned_bottom = true;
                ESP_LOGW(TAG,
                         "BOTTOM sensor (GPIO%d) never read LOW after %lu samples. "
                         "Check DOUT wiring, 3V3/GND, or turn the module pot.",
                         IR_BOTTOM_GPIO, (unsigned long)sample_count);
            }
        }

        vTaskDelayUntil(&last, period);
    }
}

bool IrManager::StartTask() {
    if (!initialized_) return false;
    BaseType_t ok = xTaskCreatePinnedToCore(
        &IrManager::TaskTrampoline, "ir_task", 2560, this,
        5, nullptr, tskNO_AFFINITY);
    return ok == pdPASS;
}

TofReading IrManager::Latest() const {
    TofReading r = {};
    r.front_obstacle = front_obstacle_.load(std::memory_order_relaxed);
    r.bottom_floor   = bottom_floor_.load(std::memory_order_relaxed);
    r.classification = static_cast<SafetyReason>(
        classification_.load(std::memory_order_relaxed));
    return r;
}

}  // namespace robot
