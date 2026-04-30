#ifndef ROBOT_MODE_MANAGER_H_
#define ROBOT_MODE_MANAGER_H_

#include "motion_manager.h"
#include "robot_types.h"

namespace robot {

// Routes commands by current mode and forwards to motion_manager.
// safety_manager (added in Step 6) will wrap calls to this via a veto hook.
class ModeManager {
public:
    explicit ModeManager(MotionManager* motion) : motion_(motion) {}

    void Submit(const Command& cmd);

    RobotMode mode() const { return mode_; }

    // Called by safety_manager. While emergency is latched, motions are blocked
    // (except STOP) but mode switches still work so the user can recover.
    void SetEmergency(bool latched, SafetyReason reason);

private:
    bool AcceptsMotion(CommandSource src) const;
    void ApplyMotion(MotionCmd m, uint8_t speed);

    MotionManager* motion_;
    RobotMode      mode_      = RobotMode::AI_MODE;
    bool           emergency_ = false;
    SafetyReason   last_reason_ = SafetyReason::OK;
};

}  // namespace robot

#endif  // ROBOT_MODE_MANAGER_H_
