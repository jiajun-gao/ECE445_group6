#pragma once

// ===== image / camera =====
#define IMG_W                 320
#define IMG_H                 240
#define CAM_FPS_TARGET        15

// ===== control bands (normalized error in [-1, +1]) =====
#define DEAD_ZONE             0.10f
#define DEAD_ZONE_EXIT        0.15f   // hysteresis: must exceed this to leave center
#define SOFT_TURN             0.30f
#define SPIN_EXIT             0.25f   // hysteresis: stop spinning when |err| < this

// ===== distance (face width / image width) =====
#define FAR_RATIO             0.15f
#define NEAR_RATIO            0.30f

// ===== P-control gains (tune on your robot) =====
#define KP_TURN               180.0f
#define KF_FWD                400.0f
#define MAX_TURN_PWM          200
#define MAX_FWD_PWM           180

// ===== timeouts (frames at ~10 FPS) =====
#define LOST_STOP_FRAMES      10      // ~1 s -> stop
#define LOST_SEARCH_FRAMES    30      // ~3 s -> begin searching
#define LOST_GIVEUP_FRAMES    80      // ~8 s -> IDLE

// ===== safety =====
#define SAFETY_TASK_HZ        100
#define EDGE_IR_ACTIVE_LOW    1       // depends on your module
#define FRONT_IR_ACTIVE_LOW   1

// ===== detector selection =====
#define DETECTOR_FACE         0
#define DETECTOR_COLOR        1
#define DEFAULT_DETECTOR      DETECTOR_FACE
