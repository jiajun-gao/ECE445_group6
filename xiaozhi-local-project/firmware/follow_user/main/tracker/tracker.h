#pragma once
#include "common.h"

typedef struct {
    target_t last;          // last valid target
    uint32_t lost_frames;
    bool     centered;      // hysteretic
    int8_t   spin_dir;      // -1 left, +1 right, 0 none (hysteretic)
} tracker_t;

void tracker_init(tracker_t *t);
// Ingest a raw detection (valid=false if none this frame). Updates filters.
void tracker_update(tracker_t *t, const target_t *det);

// Derived signals for controller. err_norm in [-1,+1], size_ratio >= 0.
bool tracker_get_error(const tracker_t *t, float *err_norm, float *size_ratio);
