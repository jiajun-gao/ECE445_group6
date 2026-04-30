#ifndef ROBOT_MOTION_MANAGER_H_
#define ROBOT_MOTION_MANAGER_H_

#include <stdint.h>

#include "esp_timer.h"

#include "motor_control.h"
#include "robot_types.h"

namespace robot {

// High-level motion primitives. The single entry point is Execute(cmd),
// which the mode_manager calls after safety checks.
//
// Turning style: we use "arc turn" for LEFT/RIGHT (inside wheel 0, outside
// wheel full speed). This gives a gentle desk-safe curve. SPIN_LEFT/RIGHT
// drives wheels in opposite directions for in-place rotation.
//
// Auto-stop safety: desk-robot voice commands ("forward", "left", etc.)
// are open-loop — the LLM asks us to start moving but there is no
// guarantee it will ever send a "stop". If the user loses Wi-Fi or stops
// paying attention, the robot would otherwise just roll off the desk.
// So every non-STOP command latches a single-shot timer that brakes the
// motors after kAutoStopMs (5 s by default). Another motion command
// before the timer fires resets the timer; an explicit STOP cancels it.
// Dance / follow modes bypass MotionManager entirely and drive motors on
// their own control tasks, so this does NOT affect those routines.
class MotionManager {
public:
    explicit MotionManager(MotorControl* motors) : motors_(motors) {}
    ~MotionManager();

    bool Init();

    // Execute a MotionCmd. If speed_percent == 0, uses default_speed_.
    void Execute(MotionCmd cmd, uint8_t speed_percent = 0);

    void SetDefaultSpeed(uint8_t pct) { default_speed_ = pct; }
    MotionCmd last_cmd() const { return last_cmd_; }

    // Duration after which a latched motion auto-stops if no further
    // command arrives. Default 5 s; adjust via SetAutoStopMs if you want
    // e.g. a slower demo pace. 0 disables auto-stop (not recommended).
    void SetAutoStopMs(uint32_t ms) { auto_stop_ms_ = ms; }
    uint32_t auto_stop_ms() const { return auto_stop_ms_; }

private:
    // esp_timer callback trampoline — runs on the esp_timer task, NOT on
    // the caller's thread. Keep the critical section short.
    static void AutoStopThunk(void* arg);
    void OnAutoStopTick();

    void CancelAutoStopTimer();
    void RestartAutoStopTimer();

    MotorControl* motors_;
    uint8_t default_speed_ = 70;   // sensible default for a light desk bot
    uint8_t turn_speed_    = 60;   // slower spins for accuracy
    MotionCmd last_cmd_    = MotionCmd::STOP;

    esp_timer_handle_t auto_stop_timer_ = nullptr;
    uint32_t           auto_stop_ms_    = 5000;
};

}  // namespace robot

#endif  // ROBOT_MOTION_MANAGER_H_
