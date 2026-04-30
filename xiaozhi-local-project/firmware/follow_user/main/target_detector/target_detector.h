#pragma once
#include "common.h"
#include "esp_camera.h"

typedef enum { DET_KIND_FACE, DET_KIND_COLOR } detector_kind_t;

void detector_init(detector_kind_t k);
// Runs the detector on `fb` and fills `out`. Sets out->valid=false if nothing.
void detector_run(camera_fb_t *fb, target_t *out);
