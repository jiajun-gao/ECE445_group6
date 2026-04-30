#include "camera_manager.h"
#include "esp_log.h"

static const char *TAG = "cam";

// Pin mapping from the board schematic (ESP32-S3-WROOM-1-N16R8 + OV5640, 8-bit DVP).
// Schematic labels D2..D9 are the sensor's 8 data lines; the esp32-camera driver
// calls them D0..D7 (8 MSBs used). Mapping: sensor Dn -> driver D(n-2).
#define CAM_PIN_PWDN    -1     // not wired
#define CAM_PIN_RESET   39
#define CAM_PIN_XCLK    36
#define CAM_PIN_SIOD    38     // SDA
#define CAM_PIN_SIOC    1      // SCL
#define CAM_PIN_VSYNC   2
#define CAM_PIN_HREF    37
#define CAM_PIN_PCLK    43

#define CAM_PIN_D7      44     // sensor D9 (MSB)
#define CAM_PIN_D6      35     // sensor D8
#define CAM_PIN_D5      42     // sensor D7
#define CAM_PIN_D4      48     // sensor D6
#define CAM_PIN_D3      41     // sensor D5
#define CAM_PIN_D2      47     // sensor D4
#define CAM_PIN_D1      40     // sensor D3
#define CAM_PIN_D0      21     // sensor D2 (LSB)

esp_err_t camera_manager_init(void) {
    camera_config_t cfg = {
        .pin_pwdn  = CAM_PIN_PWDN,  .pin_reset = CAM_PIN_RESET,
        .pin_xclk  = CAM_PIN_XCLK,
        .pin_sccb_sda = CAM_PIN_SIOD, .pin_sccb_scl = CAM_PIN_SIOC,
        .pin_d7 = CAM_PIN_D7, .pin_d6 = CAM_PIN_D6, .pin_d5 = CAM_PIN_D5,
        .pin_d4 = CAM_PIN_D4, .pin_d3 = CAM_PIN_D3, .pin_d2 = CAM_PIN_D2,
        .pin_d1 = CAM_PIN_D1, .pin_d0 = CAM_PIN_D0,
        .pin_vsync = CAM_PIN_VSYNC, .pin_href = CAM_PIN_HREF,
        .pin_pclk  = CAM_PIN_PCLK,
        .xclk_freq_hz = 20000000,
        .ledc_timer   = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,
        .pixel_format = PIXFORMAT_RGB565,   // switch to GRAYSCALE if using esp-dl face
        .frame_size   = FRAMESIZE_QVGA,     // 320x240
        .jpeg_quality = 12,
        .fb_count     = 2,
        .fb_location  = CAMERA_FB_IN_PSRAM,
        .grab_mode    = CAMERA_GRAB_LATEST,
    };
    esp_err_t err = esp_camera_init(&cfg);
    if (err != ESP_OK) { ESP_LOGE(TAG, "init fail: 0x%x", err); return err; }

    sensor_t *s = esp_camera_sensor_get();
    if (s) ESP_LOGI(TAG, "sensor PID=0x%02x", s->id.PID);  // OV5640 PID = 0x56
    return ESP_OK;
}

camera_fb_t *camera_manager_get(void) { return esp_camera_fb_get(); }
