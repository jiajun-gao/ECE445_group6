#ifndef ROBOT_FOLLOW_MANAGER_H_
#define ROBOT_FOLLOW_MANAGER_H_

#include <atomic>
#include <cstdint>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ir_manager.h"
#include "motor_control.h"

namespace robot {

// Face moods published by FollowManager. Priority of these over normal face
// state is arbitrated by FaceLcdDisplay::RecomputeEffectiveState(); see
// face_display.cc for the exact rule. Values are stable — the face display
// side mirrors this enum.
enum class FollowMood : uint8_t {
    kNone = 0,     // clear override; face returns to normal voice/music state
    kWaitHand,     // "eyes forward, slow blink" — waiting for a hand
    kActive,       // "focused eyes, small bounce" — actively following
    kTooClose,     // "surprised eyes" — hand too close (analog variant only)
    kLost,         // "confused eyes" — lost the hand
};

// State machine phases. Public because some server-facing MCP tools may
// want to report the current phase back to the LLM.
//   kIdle      : manager is stopped (not consuming the motors)
//   kWaitHand  : in follow mode, waiting for the first stable hand detection
//   kActive    : hand was stable for long enough; driving motors forward
//   kLost      : hand disappeared; motors braked, waiting for it to return
enum class FollowPhase : uint8_t {
    kIdle = 0,
    kWaitHand,
    kActive,
    kLost,
};

// FollowManager owns a dedicated FreeRTOS task (core 1, 3 KB stack) that
// implements Follow Mode for the desk robot.
//
// Hardware assumption: the board has ONE front IR distance sensor. On the
// ece445-custom-s3 that sensor is a TCRT5000 with a DIGITAL comparator
// output (GPIO15 via IrManager). "Hand present" == TofReading.front_obstacle.
// Analog distance is not available, so the proportional-control path from
// the original spec degenerates to a **constant slow forward speed** while
// a hand is present, with all timing thresholds honored (300 ms stable to
// engage, 700 ms absent to declare lost, etc.). If an analog sensor is
// added later, swap the `hand_present` flag inside TaskLoop() for a
// distance_cm reading and plug in the Kp * error control.
//
// Lifecycle:
//   Init()   — wire motors + IR + face callback. Must be called once at boot.
//   Start()  — accepted if not already running. Transitions to WAIT_HAND.
//   Stop()   — request a graceful exit; motors are braked before the task
//              deletes itself.
//
// Safety net: the task reads bottom_floor on every tick; losing the desk
// surface (cliff) aborts follow mode immediately regardless of phase.
class FollowManager {
public:
    using MoodCallback = void (*)(FollowMood);

    FollowManager() = default;
    ~FollowManager();

    bool Init(MotorControl* motors, IrManager* ir, MoodCallback mood_cb);
    bool Start();
    void Stop();

    bool running() const { return running_.load(); }
    FollowPhase phase() const { return phase_.load(); }

private:
    static void TaskEntry(void* arg);
    void TaskLoop();

    void PublishMood(FollowMood m);
    void SetPhase(FollowPhase p) { phase_.store(p); }

    MotorControl*     motors_    = nullptr;
    IrManager*        ir_        = nullptr;
    MoodCallback      mood_cb_   = nullptr;
    TaskHandle_t      task_      = nullptr;
    std::atomic<bool>        running_   {false};
    std::atomic<bool>        abort_req_ {false};
    std::atomic<FollowPhase> phase_     {FollowPhase::kIdle};
};

}  // namespace robot

#endif  // ROBOT_FOLLOW_MANAGER_H_
