#ifndef ROBOT_TYPES_H_
#define ROBOT_TYPES_H_

#include <stdint.h>

namespace robot {

enum class MotionCmd : uint8_t {
    NONE = 0,
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    SPIN_LEFT,
    SPIN_RIGHT,
    STOP,
};

enum class ModeCmd : uint8_t {
    NONE = 0,
    SWITCH_AI,
    SWITCH_MANUAL,
};

enum class RobotMode : uint8_t {
    AI_MODE = 0,
    MANUAL_MODE,
    EMERGENCY_STOP,
};

enum class CommandSource : uint8_t {
    AI_VOICE = 0,
    WIFI_MANUAL,
    SAFETY,
    SYSTEM,
};

// Reason codes used when safety_manager vetoes a motion.
//   FRONT_OBSTACLE: front-facing IR sees something within range.
//   CLIFF_DETECTED: bottom-facing IR lost the desk surface.
enum class SafetyReason : uint8_t {
    OK = 0,
    FRONT_OBSTACLE,
    CLIFF_DETECTED,
};

struct Command {
    MotionCmd motion;
    ModeCmd   mode;
    CommandSource source;
    uint8_t   speed_percent;  // 0..100; 0 means use default
};

// Named "TofReading" for legacy reasons; actually holds debounced digital
// TCRT5000 DO state.
//   front_obstacle: true => something is close in front of the robot.
//   bottom_floor  : true => desk surface is present under the robot.
struct TofReading {
    bool front_obstacle;
    bool bottom_floor;
    SafetyReason classification;
};

}  // namespace robot

#endif  // ROBOT_TYPES_H_
