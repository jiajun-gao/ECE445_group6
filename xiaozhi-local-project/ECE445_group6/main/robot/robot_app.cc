#include "robot_app.h"

#include <cJSON.h>

#include "esp_log.h"
#include "mcp_server.h"
#include "board.h"
#include "display/display.h"
#include "boards/ece445-custom-s3/face_display.h"

namespace robot {

static constexpr const char* TAG = "robot_app";

namespace {
// Whether this command actually spins the wheels. STOP / NONE should clear
// the "moving" face; everything else (forward, left, arc, spin, …) sets it.
bool IsActiveMotion(MotionCmd m) {
    return m != MotionCmd::NONE && m != MotionCmd::STOP;
}
}  // namespace

bool RobotApp::Start() {
    if (!motors_.Init())  { ESP_LOGE(TAG, "motors init failed");  return false; }
    if (!motion_.Init())  { ESP_LOGE(TAG, "motion init failed");  return false; }
    if (!ir_.Init())      { ESP_LOGE(TAG, "ir init failed");      return false; }
    if (!ir_.StartTask()) { ESP_LOGE(TAG, "ir task failed");      return false; }
    if (!safety_.StartTask()) { ESP_LOGE(TAG, "safety task failed"); return false; }
    if (!dance_.Init(&motors_, &RobotApp::OnDanceMoodChanged)) {
        ESP_LOGW(TAG, "dance init skipped (motors unavailable)");
    }
    if (!follow_.Init(&motors_, &ir_, &RobotApp::OnFollowMoodChanged)) {
        ESP_LOGW(TAG, "follow init skipped (motors/ir unavailable)");
    }
    RegisterMcpTools();
    ESP_LOGI(TAG, "robot ready (AI mode)");
    return true;
}

// Trampoline that the DanceManager's background task calls on every step.
// The enum values in robot::DanceMood are intentionally identical to the
// ones in FaceLcdDisplay::DanceMood so we can static_cast 1:1. If the face
// hasn't been built yet (unlikely after RobotApp::Start) we silently skip.
void RobotApp::OnDanceMoodChanged(DanceMood mood) {
    auto* display = Board::GetInstance().GetDisplay();
    auto* face = dynamic_cast<FaceLcdDisplay*>(display);
    if (face == nullptr) return;
    face->SetDanceMood(static_cast<FaceLcdDisplay::DanceMood>(mood));
}

// Same pattern as the dance trampoline. robot::FollowMood mirrors
// FaceLcdDisplay::FollowMood so a single static_cast is all we need.
void RobotApp::OnFollowMoodChanged(FollowMood mood) {
    auto* display = Board::GetInstance().GetDisplay();
    auto* face = dynamic_cast<FaceLcdDisplay*>(display);
    if (face == nullptr) return;
    face->SetFollowMood(static_cast<FaceLcdDisplay::FollowMood>(mood));
}

void RobotApp::Submit(const Command& in) {
    Command c = safety_.Filter(in);

    // Bring up / tear down manual HTTP server on mode transitions.
    mode_mgr_.Submit(c);
    RobotMode m = mode_mgr_.mode();
    if (m != last_seen_mode_) {
        if (m == RobotMode::MANUAL_MODE) {
            web_.Start();
        } else {
            web_.Stop();
        }
        last_seen_mode_ = m;
    }

    // Update the robot face. We use the command we actually dispatched
    // (post-safety-filter, post-mode-gate) so that backoff / e-stop /
    // manual-mode rejection leaves the face idle even if the LLM tried
    // to drive forward.
    if (auto* display = Board::GetInstance().GetDisplay()) {
        display->SetMoving(IsActiveMotion(c.motion));
    }
}

void RobotApp::SubmitVoicePhrase(const std::string& phrase) {
    Command c = VoiceCommandParser::ParsePhrase(phrase);
    if (c.motion == MotionCmd::NONE && c.mode == ModeCmd::NONE) {
        ESP_LOGD(TAG, "unrecognized phrase: '%s'", phrase.c_str());
        return;
    }
    Submit(c);
}

void RobotApp::SubmitVoiceIntent(const Command& cmd) { Submit(cmd); }

// --- MCP tool registration ---------------------------------------------------

static MotionCmd McpNameToMotion(const std::string& name) {
    if (name == "forward")    return MotionCmd::FORWARD;
    if (name == "backward")   return MotionCmd::BACKWARD;
    if (name == "left")       return MotionCmd::LEFT;
    if (name == "right")      return MotionCmd::RIGHT;
    if (name == "spin_left")  return MotionCmd::SPIN_LEFT;
    if (name == "spin_right") return MotionCmd::SPIN_RIGHT;
    if (name == "stop")       return MotionCmd::STOP;
    return MotionCmd::NONE;
}

static bool IsForwardLike(MotionCmd m) {
    return m == MotionCmd::FORWARD ||
           m == MotionCmd::LEFT ||
           m == MotionCmd::RIGHT;
}

static const char* ModeToString(RobotMode mode) {
    switch (mode) {
        case RobotMode::AI_MODE: return "ai";
        // User-facing name is "human_control" (formerly "manual"). The
        // internal enum keeps the MANUAL_MODE identifier so that existing
        // infra — wifi_manual_control, ModeCmd::SWITCH_MANUAL, mode_manager
        // transitions — stays untouched.
        case RobotMode::MANUAL_MODE: return "human_control";
        case RobotMode::EMERGENCY_STOP: return "estop";
    }
    return "unknown";
}

static const char* SafetyReasonToString(SafetyReason reason) {
    switch (reason) {
        case SafetyReason::OK: return "ok";
        case SafetyReason::FRONT_OBSTACLE: return "front_obstacle";
        case SafetyReason::CLIFF_DETECTED: return "cliff_detected";
    }
    return "unknown";
}

void RobotApp::RegisterMcpTools() {
    auto& mcp = McpServer::GetInstance();

    // robot.move — the main one. The LLM picks a direction keyword.
    PropertyList move_props({
        Property("direction", kPropertyTypeString),                 // required
        Property("speed",     kPropertyTypeInteger, 0, 0, 100),      // optional
    });
    mcp.AddTool("robot.move",
        "Move the desk robot. direction: forward|backward|left|right|spin_left|spin_right|stop. "
        "speed: 0-100 (0 = default).",
        move_props,
        [this](const PropertyList& p) -> ReturnValue {
            std::string dir = p["direction"].value<std::string>();
            int speed = 0;
            try { speed = p["speed"].value<int>(); } catch (...) {}
            Command c = {};
            c.source = CommandSource::AI_VOICE;
            c.motion = McpNameToMotion(dir);
            c.speed_percent = static_cast<uint8_t>(speed);
            if (c.motion == MotionCmd::NONE) return std::string("unknown direction");
            Submit(c);

            TofReading r = ir_.Latest();
            RobotMode mode = mode_mgr_.mode();
            SafetyReason safety_reason = safety_.latched() ? safety_.reason() : r.classification;

            cJSON* reply = cJSON_CreateObject();
            cJSON_AddStringToObject(reply, "mode", ModeToString(mode));
            cJSON_AddNumberToObject(reply, "front_obstacle", r.front_obstacle ? 1 : 0);
            cJSON_AddNumberToObject(reply, "bottom_floor", r.bottom_floor ? 1 : 0);
            cJSON_AddStringToObject(reply, "reason", SafetyReasonToString(safety_reason));

            if (mode == RobotMode::MANUAL_MODE && c.motion != MotionCmd::STOP) {
                cJSON_AddStringToObject(reply, "action", "RESPONSE");
                cJSON_AddStringToObject(reply, "response", "Robot blocked: it is currently in human control mode.");
                return reply;
            }
            if (safety_.backing_off() && c.motion != MotionCmd::STOP) {
                cJSON_AddStringToObject(reply, "action", "RESPONSE");
                cJSON_AddStringToObject(reply, "response",
                    "Robot is backing off from a detected hazard; try again in a moment.");
                return reply;
            }
            if (safety_.latched() && c.motion != MotionCmd::STOP) {
                cJSON_AddStringToObject(reply, "action", "RESPONSE");
                cJSON_AddStringToObject(reply, "response", "Robot blocked: cliff safety is latched.");
                return reply;
            }
            if (safety_reason == SafetyReason::FRONT_OBSTACLE && IsForwardLike(c.motion)) {
                cJSON_AddStringToObject(reply, "action", "RESPONSE");
                cJSON_AddStringToObject(reply, "response", "Robot blocked: front obstacle detected.");
                return reply;
            }

            cJSON_AddStringToObject(reply, "action", "RESPONSE");
            cJSON_AddStringToObject(reply, "response", "Robot motion accepted.");
            return reply;
        });

    // robot.set_mode — AI / human_control
    PropertyList mode_props({ Property("mode", kPropertyTypeString) });
    mcp.AddTool("robot.set_mode",
        "Switch robot mode. mode: ai | human_control. In human control mode a "
        "Wi-Fi web controller appears so a human can drive the robot by hand.",
        mode_props,
        [this](const PropertyList& p) -> ReturnValue {
            std::string m = p["mode"].value<std::string>();
            Command c = {};
            c.source = CommandSource::AI_VOICE;
            if      (m == "ai")     c.mode = ModeCmd::SWITCH_AI;
            // Primary user-facing identifier is "human_control". We also
            // accept "human" and the legacy "manual" so older clients and
            // LLM turns that still use the old name keep working.
            else if (m == "human_control" || m == "human" || m == "manual") {
                c.mode = ModeCmd::SWITCH_MANUAL;
            } else {
                return std::string("unknown mode");
            }
            Submit(c);
            return std::string("ok");
        });

    // robot.dance — plays the built-in dance routine on a dedicated
    // FreeRTOS task and returns IMMEDIATELY with the estimated duration.
    // The server uses "duration_ms" to schedule the rating question after
    // the dance ends (see server-side dance handler). This tool is a
    // no-op while the robot is in human_control mode or the safety
    // system is latched; those checks return a natural-language reason.
    mcp.AddTool("robot.dance",
        "Play the built-in dance routine. The robot performs a short "
        "~15-second choreography with synchronized face expressions. Returns "
        "immediately; the dance runs in the background on the device. Only "
        "call this when the user explicitly asks the robot to dance or "
        "perform; do not call it otherwise.",
        PropertyList(),
        [this](const PropertyList&) -> ReturnValue {
            cJSON* reply = cJSON_CreateObject();
            RobotMode mode = mode_mgr_.mode();
            if (mode == RobotMode::MANUAL_MODE) {
                cJSON_AddStringToObject(reply, "action", "RESPONSE");
                cJSON_AddStringToObject(reply, "response",
                    "I cannot dance while I am in human control mode.");
                return reply;
            }
            if (safety_.latched()) {
                cJSON_AddStringToObject(reply, "action", "RESPONSE");
                cJSON_AddStringToObject(reply, "response",
                    "Cliff safety is latched; I cannot dance right now.");
                return reply;
            }
            uint32_t dur_ms = 0;
            bool ok = dance_.Start(&dur_ms);
            if (!ok) {
                cJSON_AddStringToObject(reply, "action", "RESPONSE");
                cJSON_AddStringToObject(reply, "response",
                    "I am already dancing.");
                cJSON_AddNumberToObject(reply, "duration_ms",
                                        DanceManager::PresetDurationMs());
                cJSON_AddBoolToObject(reply, "started", false);
                return reply;
            }
            cJSON_AddStringToObject(reply, "action", "RESPONSE");
            cJSON_AddStringToObject(reply, "response",
                "Watch my dance!");
            cJSON_AddNumberToObject(reply, "duration_ms", dur_ms);
            cJSON_AddBoolToObject(reply, "started", true);
            return reply;
        });

    // robot.dance_stop — early abort for the dance routine.
    mcp.AddTool("robot.dance_stop",
        "Stop the dance routine immediately if one is currently playing.",
        PropertyList(),
        [this](const PropertyList&) -> ReturnValue {
            dance_.Stop();
            return std::string("ok");
        });

    // robot.follow_start — enter Follow Mode. The device runs a dedicated
    // FreeRTOS task that watches the front IR sensor and drives the motors
    // forward slowly while a hand (or any object) stays within the sensor's
    // range. Returns IMMEDIATELY; the follow routine runs in the background.
    // Only call this when the user explicitly asks the robot to follow
    // (e.g. "follow me", "follow my hand"); do not call it otherwise.
    mcp.AddTool("robot.follow_start",
        "Enter Follow Mode: the robot watches the front IR sensor and moves "
        "slowly toward a detected hand. Returns immediately; follow runs in "
        "the background. Call robot.follow_stop to exit. Refuses while in "
        "human_control mode or with the cliff safety latched.",
        PropertyList(),
        [this](const PropertyList&) -> ReturnValue {
            cJSON* reply = cJSON_CreateObject();
            RobotMode mode = mode_mgr_.mode();
            if (mode == RobotMode::MANUAL_MODE) {
                cJSON_AddStringToObject(reply, "action", "RESPONSE");
                cJSON_AddStringToObject(reply, "response",
                    "I cannot follow while I am in human control mode.");
                cJSON_AddBoolToObject(reply, "started", false);
                return reply;
            }
            if (safety_.latched()) {
                cJSON_AddStringToObject(reply, "action", "RESPONSE");
                cJSON_AddStringToObject(reply, "response",
                    "Cliff safety is latched; I cannot follow right now.");
                cJSON_AddBoolToObject(reply, "started", false);
                return reply;
            }
            if (!follow_.Start()) {
                cJSON_AddStringToObject(reply, "action", "RESPONSE");
                cJSON_AddStringToObject(reply, "response",
                    "I am already in follow mode.");
                cJSON_AddBoolToObject(reply, "started", false);
                return reply;
            }
            cJSON_AddStringToObject(reply, "action", "RESPONSE");
            cJSON_AddStringToObject(reply, "response",
                "Okay, I will follow your hand.");
            cJSON_AddBoolToObject(reply, "started", true);
            return reply;
        });

    // robot.follow_stop — graceful exit from Follow Mode (brakes motors,
    // clears face mood). Safe to call even when not following.
    mcp.AddTool("robot.follow_stop",
        "Stop Follow Mode if it is currently active. Brakes motors and "
        "returns the face to the normal state. Safe to call in any state.",
        PropertyList(),
        [this](const PropertyList&) -> ReturnValue {
            bool was_running = follow_.running();
            follow_.Stop();
            cJSON* reply = cJSON_CreateObject();
            cJSON_AddStringToObject(reply, "action", "RESPONSE");
            cJSON_AddStringToObject(reply, "response",
                was_running ? "Follow mode stopped." : "I am not following.");
            cJSON_AddBoolToObject(reply, "was_running", was_running);
            return reply;
        });

    // robot.get_status — safety + IR introspection, useful for the LLM.
    mcp.AddTool("robot.get_status",
        "Report robot mode, safety latch state and latest IR readings.",
        PropertyList(),
        [this](const PropertyList&) -> ReturnValue {
            TofReading r = ir_.Latest();
            std::string s = "mode=";
            s += (mode_mgr_.mode() == RobotMode::AI_MODE ? "ai" :
                  mode_mgr_.mode() == RobotMode::MANUAL_MODE ? "human_control" : "estop");
            s += " front_obstacle=" + std::to_string(r.front_obstacle ? 1 : 0);
            s += " bottom_floor="   + std::to_string(r.bottom_floor   ? 1 : 0);
            s += " reason=" + std::to_string(static_cast<int>(r.classification));
            return s;
        });
}

}  // namespace robot
