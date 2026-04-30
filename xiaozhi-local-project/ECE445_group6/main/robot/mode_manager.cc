#include "mode_manager.h"

#include "esp_log.h"

namespace robot {

static constexpr const char* TAG = "mode";

bool ModeManager::AcceptsMotion(CommandSource src) const {
    switch (mode_) {
        case RobotMode::AI_MODE:        return src == CommandSource::AI_VOICE ||
                                               src == CommandSource::SAFETY;
        case RobotMode::MANUAL_MODE:    return src == CommandSource::WIFI_MANUAL ||
                                               src == CommandSource::SAFETY;
        case RobotMode::EMERGENCY_STOP: return src == CommandSource::SAFETY;
    }
    return false;
}

void ModeManager::ApplyMotion(MotionCmd m, uint8_t speed) {
    if (!motion_) return;
    // STOP is always allowed.
    if (emergency_ && m != MotionCmd::STOP) {
        ESP_LOGW(TAG, "blocked by emergency (reason=%d)", static_cast<int>(last_reason_));
        motion_->Execute(MotionCmd::STOP);
        return;
    }
    motion_->Execute(m, speed);
}

void ModeManager::Submit(const Command& cmd) {
    // Mode switches first.
    if (cmd.mode == ModeCmd::SWITCH_MANUAL && mode_ != RobotMode::MANUAL_MODE) {
        mode_ = RobotMode::MANUAL_MODE;
        motion_->Execute(MotionCmd::STOP);
        ESP_LOGI(TAG, "-> MANUAL_MODE");
        return;
    }
    if (cmd.mode == ModeCmd::SWITCH_AI && mode_ != RobotMode::AI_MODE) {
        mode_ = RobotMode::AI_MODE;
        motion_->Execute(MotionCmd::STOP);
        ESP_LOGI(TAG, "-> AI_MODE");
        return;
    }

    if (cmd.motion == MotionCmd::NONE) return;

    // STOP always obeyed regardless of mode.
    if (cmd.motion == MotionCmd::STOP) {
        motion_->Execute(MotionCmd::STOP);
        return;
    }

    if (!AcceptsMotion(cmd.source)) {
        ESP_LOGD(TAG, "drop src=%d in mode=%d",
                 static_cast<int>(cmd.source), static_cast<int>(mode_));
        return;
    }
    ApplyMotion(cmd.motion, cmd.speed_percent);
}

void ModeManager::SetEmergency(bool latched, SafetyReason reason) {
    if (latched && !emergency_) {
        ESP_LOGE(TAG, "EMERGENCY latched reason=%d", static_cast<int>(reason));
        motion_->Execute(MotionCmd::STOP);
    } else if (!latched && emergency_) {
        ESP_LOGI(TAG, "emergency cleared");
    }
    emergency_   = latched;
    last_reason_ = reason;
}

}  // namespace robot
