#include "mode_manager.h"
#include "follow_controller.h"
#include "motor_control.h"
#include "esp_log.h"

static const char *TAG = "mode";
static mode_t s_mode = MODE_IDLE;

void mode_init(void) { s_mode = MODE_IDLE; }
mode_t mode_get(void) { return s_mode; }

void mode_set(mode_t m) {
    if (m == s_mode) return;
    ESP_LOGI(TAG, "%d -> %d", s_mode, m);

    // Leaving FOLLOW: disable controller
    if (s_mode == MODE_FOLLOW_USER && m != MODE_FOLLOW_USER) {
        follow_enable(false);
    }
    // Entering FOLLOW: enable controller
    if (m == MODE_FOLLOW_USER) {
        follow_enable(true);
    }
    // Any transition: stop motors as a safe handoff
    motor_cmd_t stop = { .verb = CMD_STOP, .pwm = 0, .source = CMD_SRC_MANUAL };
    motor_control_submit(&stop);

    s_mode = m;
}

void mode_submit_external(const motor_cmd_t *cmd) {
    // Always allow STOP.
    if (cmd->verb == CMD_STOP) { motor_control_submit(cmd); return; }

    switch (s_mode) {
        case MODE_MANUAL:
            if (cmd->source & CMD_SRC_MANUAL) motor_control_submit(cmd);
            break;
        case MODE_AI:
            if (cmd->source & CMD_SRC_AI)     motor_control_submit(cmd);
            break;
        case MODE_FOLLOW_USER:
            // Only follow_controller drives motion in this mode.
            ESP_LOGD(TAG, "drop external cmd in FOLLOW mode");
            break;
        case MODE_SAFE_STOP:
        case MODE_IDLE:
        default:
            ESP_LOGD(TAG, "drop external cmd in mode %d", s_mode);
            break;
    }
}
