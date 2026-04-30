#pragma once
#include "common.h"

void safety_init(int front_ir_gpio, int edge_ir_gpio);
void safety_start(void);                   // spawns 100Hz task
safety_state_t safety_get(void);           // latest snapshot
bool safety_override_active(void);         // true => motor_control must obey safety
