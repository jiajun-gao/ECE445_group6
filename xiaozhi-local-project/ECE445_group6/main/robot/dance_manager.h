#ifndef ROBOT_DANCE_MANAGER_H_
#define ROBOT_DANCE_MANAGER_H_

#include <atomic>
#include <cstdint>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "motor_control.h"

namespace robot {

// Face "moods" for the dance sequence. Maps to FaceLcdDisplay::DanceMood;
// kept here so the dance manager only needs a small enum, and we don't
// couple motor logic to LVGL headers. Values are stable across both sides.
enum class DanceMood : uint8_t {
    kNone = 0,        // clear dance mood, fall back to regular face state
    kReady,           // pre-dance, head-tilt listen pose
    kExcited,         // main dance, wide/curved eyes
    kSpin,            // spin phase, eyes shift with rotation
    kHighlight,       // climax, big eyes / fast blink
    kEndPose,         // big happy smile
    kSad,             // bad score reaction
    kEmbarrassed,     // mid score reaction
};

// A single step in the preloaded dance sequence. Speeds are signed:
//   +100 = full forward on that wheel, -100 = full reverse, 0 = stop.
// duration_ms is the time to *hold* this step before moving to the next one.
// mood is written to the face on step entry (non-blocking).
struct DanceStep {
    int8_t   left_speed;
    int8_t   right_speed;
    uint16_t duration_ms;
    DanceMood mood;
};

// DanceManager owns a single background FreeRTOS task that runs a preloaded
// dance sequence on the motors. Face updates are published via a callback
// (supplied by RobotApp::Start()) so this file never pulls in the LVGL face
// headers. The task is non-blocking from the caller's perspective:
//
//   Start()  → immediately returns "accepted" / "busy"; motors and face
//              are driven on the internal task until the sequence ends.
//   Stop()   → safely stops the task, brakes motors, clears face mood.
//   running()→ true while the routine is active.
//
// The task runs on a small stack (3 KB) pinned to core 1 so LVGL (core 0)
// and the Wi-Fi / MCP stack never stutter.
class DanceManager {
public:
    using MoodCallback = void (*)(DanceMood mood);

    DanceManager() = default;
    ~DanceManager();

    // Must be called before Start(). motors must outlive DanceManager.
    // mood_cb is invoked on every step entry + on sequence end with kNone.
    // It's allowed to be nullptr if the caller doesn't care about face sync.
    bool Init(MotorControl* motors, MoodCallback mood_cb);

    // Kick off the dance. Returns false if already running. Steps are
    // read from the built-in preset defined in dance_manager.cc. The
    // estimated total duration in ms is written to *out_duration_ms if
    // non-null so the caller (MCP tool, server) can schedule a follow-up
    // prompt ("how would you rate my dance?") at the right time.
    bool Start(uint32_t* out_duration_ms = nullptr);

    // Request early abort. Safe to call from any task. Returns promptly.
    // Motors are braked before the task exits.
    void Stop();

    bool running() const { return running_.load(); }

    // Duration of the built-in sequence, in milliseconds. Useful for
    // server-side scheduling even before Start() is called.
    static uint32_t PresetDurationMs();

private:
    static void TaskEntry(void* arg);
    void TaskLoop();

    MotorControl*     motors_    = nullptr;
    MoodCallback      mood_cb_   = nullptr;
    TaskHandle_t      task_      = nullptr;
    std::atomic<bool> running_   {false};
    std::atomic<bool> abort_req_ {false};
};

}  // namespace robot

#endif  // ROBOT_DANCE_MANAGER_H_
