#ifndef ROBOT_SAFETY_MANAGER_H_
#define ROBOT_SAFETY_MANAGER_H_

#include "ir_manager.h"
#include "mode_manager.h"
#include "motion_manager.h"
#include "robot_types.h"

namespace robot {

// Set to 0 at build time to disable cliff latching entirely. Useful during
// bench bring-up when the bottom TCRT5000 sensor is not yet wired or its
// polarity has not been verified (see README in this folder). Front-obstacle
// veto is unaffected.
#ifndef ROBOT_CLIFF_SAFETY_ENABLED
#define ROBOT_CLIFF_SAFETY_ENABLED 1
#endif

// safety_manager sits between every command source and the mode_manager.
//
// Rules:
//   1. On the RISING EDGE of any hazard (front obstacle OR cliff), briefly
//      drive the robot BACKWARD for ~6 cm before handing off to the
//      downstream state. This gives the robot a reflexive "back off" response
//      the instant it detects trouble.
//   2. After the back-off completes:
//        * Cliff  -> latch EMERGENCY_STOP. Only STOP + mode switches pass.
//                    (disabled when ROBOT_CLIFF_SAFETY_ENABLED == 0)
//        * Front  -> return to idle; front-obstacle veto keeps blocking
//                    forward-like motion until the obstacle is gone.
//   3. Cliff latch clears after HAZARD_CLEAR_MS of OK readings AND a STOP cmd.
//   4. While a back-off is in progress, all external motion commands are
//      vetoed (turned into STOP) so they can't fight the reflex; STOP itself
//      still aborts it.
class SafetyManager {
public:
    SafetyManager(IrManager* ir, ModeManager* mode, MotionManager* motion)
        : ir_(ir), mode_(mode), motion_(motion) {}

    // Starts the watchdog task (checks IR every 20 ms, updates mode_).
    bool StartTask();

    // Filter a command before it reaches ModeManager::Submit.
    // Returns the (possibly STOP-replaced) command to forward.
    Command Filter(const Command& in);

    bool latched() const { return latched_; }
    SafetyReason reason() const { return reason_; }
    bool backing_off() const { return state_ == State::BACKING_OFF; }

private:
    static void TaskTrampoline(void* arg);
    void TaskLoop();

    enum class State : uint8_t { IDLE, BACKING_OFF, LATCHED };

    IrManager*     ir_;
    ModeManager*   mode_;
    MotionManager* motion_;
    bool           latched_ = false;
    SafetyReason   reason_  = SafetyReason::OK;
    uint32_t       ok_streak_ms_ = 0;
    State          state_ = State::IDLE;
    uint32_t       backoff_ms_remaining_ = 0;
    SafetyReason   backoff_reason_ = SafetyReason::OK;

    static constexpr uint32_t HAZARD_CLEAR_MS = 500;
    // Empirical: ~6 cm at BACKOFF_SPEED on a 70x50 desk bot chassis.
    // If the robot overshoots/undershoots, tune BACKOFF_MS first (cheapest),
    // then BACKOFF_SPEED. No wheel encoder, pure dead reckoning.
    static constexpr uint32_t BACKOFF_MS    = 400;
    static constexpr uint8_t  BACKOFF_SPEED = 70;
};

}  // namespace robot

#endif  // ROBOT_SAFETY_MANAGER_H_
