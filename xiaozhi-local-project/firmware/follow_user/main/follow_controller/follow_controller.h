#pragma once
#include "common.h"
#include "tracker.h"

void follow_init(void);
void follow_start(void);               // spawns control_task @ 20 Hz
void follow_feed_target(const target_t *det); // called by vision_task
void follow_enable(bool on);           // gated by mode_manager
follow_state_t follow_get_state(void);
