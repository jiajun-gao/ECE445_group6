#pragma once
#include "common.h"

// Your existing primitives — keep signatures, re-wire implementations.
void motor_init(void);
void motor_forward(int pwm);
void motor_backward(int pwm);
void motor_left(int pwm);
void motor_right(int pwm);
void motor_spin_left(int pwm);
void motor_spin_right(int pwm);
void motor_stop(void);

// New: unified queue-based entry points used by the rest of the system.
void motor_control_start(void);                       // spawns motor_task
void motor_control_submit(const motor_cmd_t *cmd);    // normal path (subject to safety + mode)
void motor_control_submit_override(const motor_cmd_t *cmd); // used ONLY by safety_manager
