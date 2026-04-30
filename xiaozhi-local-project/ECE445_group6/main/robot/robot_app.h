#ifndef ROBOT_APP_H_
#define ROBOT_APP_H_

#include "dance_manager.h"
#include "follow_manager.h"
#include "ir_manager.h"
#include "mode_manager.h"
#include "motion_manager.h"
#include "motor_control.h"
#include "safety_manager.h"
#include "voice_command_parser.h"
#include "wifi_manual_control.h"

namespace robot {

// One-stop integrator. Call RobotApp::GetInstance().Start() once after NVS
// + Wi-Fi STA are up (application.cc line ~99 is a good spot).
//
// External entry points:
//   SubmitVoicePhrase("forward")  -> parse then route
//   SubmitVoiceIntent(cmd)        -> pre-parsed from MCP tool
//   RegisterMcpTools()            -> adds robot.move / robot.stop / robot.set_mode
class RobotApp {
public:
    static RobotApp& GetInstance() { static RobotApp i; return i; }

    bool Start();

    void SubmitVoicePhrase(const std::string& phrase);
    void SubmitVoiceIntent(const Command& cmd);

    // Registers MCP tools so the LLM can control the robot directly.
    void RegisterMcpTools();

    RobotMode mode() const { return mode_mgr_.mode(); }
    TofReading ir_reading() const { return ir_.Latest(); }

private:
    RobotApp()
        : motion_(&motors_),
          mode_mgr_(&motion_),
          safety_(&ir_, &mode_mgr_, &motion_),
          web_([this](const Command& c){ Submit(c); }) {}

    // Single choke point: safety filter -> mode_mgr
    void Submit(const Command& in);

    MotorControl     motors_;
    MotionManager    motion_;
    ModeManager      mode_mgr_;
    IrManager        ir_;
    SafetyManager    safety_;
    WifiManualControl web_;
    DanceManager     dance_;
    FollowManager    follow_;
    RobotMode        last_seen_mode_ = RobotMode::AI_MODE;

    // Trampolines used by the dance + follow tasks to push their current
    // mood into the LCD face without pulling face_display.h into this
    // header. Both forward to the FaceLcdDisplay singleton.
    static void OnDanceMoodChanged(DanceMood mood);
    static void OnFollowMoodChanged(FollowMood mood);
};

}  // namespace robot

#endif  // ROBOT_APP_H_
