#include "motion_manager.h"

#include "esp_log.h"

namespace robot {

static constexpr const char* TAG = "motion";

MotionManager::~MotionManager() {
    CancelAutoStopTimer();
    if (auto_stop_timer_) {
        esp_timer_delete(auto_stop_timer_);
        auto_stop_timer_ = nullptr;
    }
}

bool MotionManager::Init() {
    if (!motors_ || !motors_->Init()) return false;

    // Lazily create the auto-stop timer here so Init() is the single
    // "everything is wired" guarantee point. The timer is never started
    // unless Execute() receives a non-STOP command.
    if (!auto_stop_timer_) {
        esp_timer_create_args_t args = {};
        args.callback = &MotionManager::AutoStopThunk;
        args.arg      = this;
        args.dispatch_method = ESP_TIMER_TASK;
        args.name     = "motion_auto_stop";
        esp_err_t err = esp_timer_create(&args, &auto_stop_timer_);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_timer_create failed: %d", err);
            auto_stop_timer_ = nullptr;
            return false;
        }
    }
    return true;
}

void MotionManager::CancelAutoStopTimer() {
    if (auto_stop_timer_ && esp_timer_is_active(auto_stop_timer_)) {
        esp_timer_stop(auto_stop_timer_);
    }
}

void MotionManager::RestartAutoStopTimer() {
    if (!auto_stop_timer_ || auto_stop_ms_ == 0) return;
    // esp_timer_start_once rejects a running timer, so cancel first.
    if (esp_timer_is_active(auto_stop_timer_)) {
        esp_timer_stop(auto_stop_timer_);
    }
    esp_err_t err = esp_timer_start_once(
        auto_stop_timer_,
        static_cast<uint64_t>(auto_stop_ms_) * 1000ULL);  // microseconds
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "esp_timer_start_once failed: %d", err);
    }
}

void MotionManager::AutoStopThunk(void* arg) {
    static_cast<MotionManager*>(arg)->OnAutoStopTick();
}

void MotionManager::OnAutoStopTick() {
    // Runs on the esp_timer task. Keep it tiny: just brake and log.
    if (!motors_) return;
    motors_->Stop(/*active_brake=*/true);
    last_cmd_ = MotionCmd::STOP;
    ESP_LOGI(TAG, "auto-stop fired after %u ms", (unsigned)auto_stop_ms_);
}

void MotionManager::Execute(MotionCmd cmd, uint8_t speed_percent) {
    if (!motors_) return;
    uint8_t s = speed_percent ? speed_percent : default_speed_;
    int8_t  fwd = static_cast<int8_t>(s);
    int8_t  rev = -static_cast<int8_t>(s);
    int8_t  turn = static_cast<int8_t>(turn_speed_);

    switch (cmd) {
        case MotionCmd::FORWARD:
            motors_->SetLeft(fwd);  motors_->SetRight(fwd); break;
        case MotionCmd::BACKWARD:
            motors_->SetLeft(rev);  motors_->SetRight(rev); break;
        case MotionCmd::LEFT:          // arc left: left wheel stops, right drives
            motors_->SetLeft(0);    motors_->SetRight(fwd); break;
        case MotionCmd::RIGHT:         // arc right
            motors_->SetLeft(fwd);  motors_->SetRight(0);   break;
        case MotionCmd::SPIN_LEFT:     // counter-rotate
            motors_->SetLeft(-turn); motors_->SetRight(turn); break;
        case MotionCmd::SPIN_RIGHT:
            motors_->SetLeft(turn);  motors_->SetRight(-turn); break;
        case MotionCmd::STOP:
            motors_->Stop(true);   break;  // active brake
        case MotionCmd::NONE:
        default:
            return;
    }
    last_cmd_ = cmd;
    ESP_LOGI(TAG, "exec cmd=%d speed=%u", static_cast<int>(cmd), s);

    // Auto-stop policy:
    //   - Any live motion command (forward/back/left/right/spin*) latches
    //     the timer. A fresh command restarts it so a user holding a
    //     "keep going" conversation isn't cut off mid-maneuver.
    //   - STOP / NONE (NONE already returned above) cancel the timer so
    //     the motors stay stopped without a spurious second brake.
    if (cmd == MotionCmd::STOP) {
        CancelAutoStopTimer();
    } else {
        RestartAutoStopTimer();
    }
}

}  // namespace robot
