#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    MODE_IDLE = 0,
    MODE_AI,
    MODE_MANUAL,
    MODE_FOLLOW_USER,
    MODE_SAFE_STOP,
} mode_t;

typedef enum {
    FOLLOW_IDLE = 0,
    FOLLOW_SEARCHING,
    FOLLOW_TRACKING,
    FOLLOW_HOLD,
} follow_state_t;

typedef enum {
    CMD_STOP = 0,
    CMD_FORWARD,
    CMD_BACKWARD,
    CMD_LEFT,         // gentle turn while moving
    CMD_RIGHT,
    CMD_SPIN_LEFT,
    CMD_SPIN_RIGHT,
} motor_verb_t;

typedef struct {
    motor_verb_t verb;
    int16_t pwm;       // 0..255, verb-specific meaning
    uint32_t source;   // bitmask: 1=follow, 2=manual, 4=ai, 8=safety
} motor_cmd_t;

typedef struct {
    bool     valid;
    float    cx, cy;       // pixel coords of target center
    float    w, h;         // pixel bbox size
    float    confidence;   // 0..1
    uint32_t ts_ms;
} target_t;

typedef struct {
    bool front_blocked;
    bool edge_detected;
} safety_state_t;

#define CMD_SRC_FOLLOW  0x01
#define CMD_SRC_MANUAL  0x02
#define CMD_SRC_AI      0x04
#define CMD_SRC_SAFETY  0x08
