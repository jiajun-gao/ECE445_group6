#pragma once
#include "esp_camera.h"

// Initialize OV5640 at QVGA RGB565. Returns ESP_OK on success.
esp_err_t camera_manager_init(void);
// Capture a frame. Caller MUST call esp_camera_fb_return(fb) when done.
camera_fb_t *camera_manager_get(void);
