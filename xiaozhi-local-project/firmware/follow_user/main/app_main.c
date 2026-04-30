#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_camera.h"

#include "config.h"
#include "common.h"
#include "camera_manager.h"
#include "target_detector.h"
#include "tracker.h"
#include "follow_controller.h"
#include "motor_control.h"
#include "safety_manager.h"
#include "mode_manager.h"

static const char *TAG = "app";

// --- vision task: camera -> detector -> follow_feed_target ------------------
static void vision_task(void *arg) {
    const TickType_t period = pdMS_TO_TICKS(1000 / 10); // 10 Hz
    for (;;) {
        camera_fb_t *fb = camera_manager_get();
        if (!fb) { vTaskDelay(period); continue; }
        target_t det;
        detector_run(fb, &det);
        det.ts_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
        follow_feed_target(&det);
        if (det.valid) {
            ESP_LOGI(TAG, "det cx=%.1f w=%.1f conf=%.2f", det.cx, det.w, det.confidence);
        }
        esp_camera_fb_return(fb);
        vTaskDelay(period);
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "boot");

    // ----- motors + safety FIRST (safety must be alive before anything moves)
    motor_init();
    motor_control_start();
    safety_init(/*front_ir_gpio=*/1, /*edge_ir_gpio=*/2);  // TODO your GPIOs
    safety_start();

    // ----- vision stack
    if (camera_manager_init() != ESP_OK) {
        ESP_LOGE(TAG, "camera init failed, aborting vision");
    } else {
        detector_init(DEFAULT_DETECTOR == DETECTOR_FACE ? DET_KIND_FACE : DET_KIND_COLOR);
        follow_init();
        follow_start();
        xTaskCreatePinnedToCore(vision_task, "vision", 6144, NULL, 6, NULL, 1);
    }

    mode_init();
    mode_set(MODE_IDLE);

    ESP_LOGI(TAG, "ready — say \"follow me\" to start");
}
