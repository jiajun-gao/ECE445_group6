#pragma once
#include "common.h"

void mode_init(void);
void mode_set(mode_t m);
mode_t mode_get(void);

// Submit a command from a non-follow source (AI, web/manual). It will be
// dropped unless the current mode allows it.
void mode_submit_external(const motor_cmd_t *cmd);
