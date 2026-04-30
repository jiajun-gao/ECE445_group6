#include "safety_manager.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace robot {

static constexpr const char* TAG = "safety";

static bool IsForwardLike(MotionCmd m) {
    return m == MotionCmd::FORWARD ||
           m == MotionCmd::LEFT    ||   // arc turns roll forward on outside wheel
           m == MotionCmd::RIGHT;
}

Command SafetyManager::Filter(const Command& in) {
    Command out = in;

    // Active reflex back-off OR cliff latch: block everything except STOP.
    // Mode switches still pass through mode_mgr so the user can recover.
    if (state_ == State::BACKING_OFF || latched_) {
        if (in.motion != MotionCmd::NONE && in.motion != MotionCmd::STOP) {
            ESP_LOGW(TAG, "veto (state=%d latched=%d reason=%d) motion=%d",
                     static_cast<int>(state_), latched_ ? 1 : 0,
                     static_cast<int>(reason_), static_cast<int>(in.motion));
            out.motion = MotionCmd::STOP;
        }
        return out;
    }

    // Front-obstacle veto: blocks forward-like motion only. Backward, spin,
    // stop still allowed -- the user can back off the obstacle.
    if (reason_ == SafetyReason::FRONT_OBSTACLE && IsForwardLike(in.motion)) {
        ESP_LOGW(TAG, "veto FRONT_OBSTACLE motion=%d", static_cast<int>(in.motion));
        out.motion = MotionCmd::STOP;
    }
    return out;
}

void SafetyManager::TaskTrampoline(void* arg) {
    static_cast<SafetyManager*>(arg)->TaskLoop();
}

void SafetyManager::TaskLoop() {
    const TickType_t period = pdMS_TO_TICKS(20);
    TickType_t last = xTaskGetTickCount();

    // Previous classification, used for rising-edge detection so we only fire
    // the back-off once per OK -> HAZARD transition instead of every 20 ms.
    SafetyReason prev = SafetyReason::OK;

    while (true) {
        TofReading r = ir_->Latest();
        SafetyReason now = r.classification;

#if !ROBOT_CLIFF_SAFETY_ENABLED
        if (now == SafetyReason::CLIFF_DETECTED) now = SafetyReason::OK;
#endif

        switch (state_) {
            case State::IDLE: {
                // Track live state so Filter() can apply the forward-veto.
                reason_ = now;

                bool rising_edge = (prev == SafetyReason::OK) &&
                                   (now == SafetyReason::CLIFF_DETECTED ||
                                    now == SafetyReason::FRONT_OBSTACLE);
                if (rising_edge) {
                    ESP_LOGW(TAG,
                             "hazard rising edge (reason=%d) -> backoff %lu ms @ %u%%",
                             static_cast<int>(now),
                             static_cast<unsigned long>(BACKOFF_MS),
                             static_cast<unsigned>(BACKOFF_SPEED));
                    backoff_reason_       = now;
                    backoff_ms_remaining_ = BACKOFF_MS;
                    state_                = State::BACKING_OFF;
                    // Direct motor drive: bypasses mode/source filtering the
                    // same way SetEmergency's STOP does. This is a reflex, not
                    // a user command.
                    motion_->Execute(MotionCmd::BACKWARD, BACKOFF_SPEED);
                }
                break;
            }

            case State::BACKING_OFF: {
                if (backoff_ms_remaining_ > 20) {
                    backoff_ms_remaining_ -= 20;
                } else {
                    backoff_ms_remaining_ = 0;
                    motion_->Execute(MotionCmd::STOP);
                    if (backoff_reason_ == SafetyReason::CLIFF_DETECTED) {
                        ESP_LOGE(TAG,
                                 "backoff done -> CLIFF LATCH -> EMERGENCY_STOP");
                        latched_      = true;
                        reason_       = SafetyReason::CLIFF_DETECTED;
                        ok_streak_ms_ = 0;
                        mode_->SetEmergency(true, SafetyReason::CLIFF_DETECTED);
                        state_ = State::LATCHED;
                    } else {
                        ESP_LOGI(TAG,
                                 "backoff done (front obstacle); resuming");
                        // Let live IR reading drive the (non-latching) veto.
                        reason_ = now;
                        state_  = State::IDLE;
                    }
                }
                break;
            }

            case State::LATCHED: {
                if (now == SafetyReason::OK) {
                    ok_streak_ms_ += 20;
                    if (ok_streak_ms_ >= HAZARD_CLEAR_MS) {
                        latched_ = false;
                        reason_  = SafetyReason::OK;
                        ESP_LOGI(TAG, "hazard cleared; emergency released");
                        mode_->SetEmergency(false, SafetyReason::OK);
                        state_ = State::IDLE;
                    }
                } else {
                    ok_streak_ms_ = 0;
                }
                break;
            }
        }

        prev = now;
        vTaskDelayUntil(&last, period);
    }
}

bool SafetyManager::StartTask() {
    BaseType_t ok = xTaskCreatePinnedToCore(
        &SafetyManager::TaskTrampoline, "safety_task", 3072, this,
        6, nullptr, tskNO_AFFINITY);
    return ok == pdPASS;
}

}  // namespace robot
