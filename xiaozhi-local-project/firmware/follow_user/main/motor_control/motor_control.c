#include "motor_control.h"
#include "safety_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

static const char *TAG = "motor";
static QueueHandle_t s_q;

static void apply(const motor_cmd_t *c) {
    switch (c->verb) {
        case CMD_STOP:        motor_stop();                 break;
        case CMD_FORWARD:     motor_forward(c->pwm);        break;
        case CMD_BACKWARD:    motor_backward(c->pwm);       break;
        case CMD_LEFT:        motor_left(c->pwm);           break;
        case CMD_RIGHT:       motor_right(c->pwm);          break;
        case CMD_SPIN_LEFT:   motor_spin_left(c->pwm);      break;
        case CMD_SPIN_RIGHT:  motor_spin_right(c->pwm);     break;
    }
}

static void motor_task(void *arg) {
    motor_cmd_t cmd;
    for (;;) {
        if (xQueueReceive(s_q, &cmd, portMAX_DELAY) == pdTRUE) {
            // Safety override: only SAFETY-sourced or STOP commands pass through.
            if (safety_override_active() &&
                !(cmd.source & CMD_SRC_SAFETY) &&
                cmd.verb != CMD_STOP) {
                ESP_LOGD(TAG, "drop verb=%d src=0x%lx (safety override)", cmd.verb,
                         (unsigned long)cmd.source);
                continue;
            }
            apply(&cmd);
        }
    }
}

void motor_control_start(void) {
    s_q = xQueueCreate(8, sizeof(motor_cmd_t));
    xTaskCreatePinnedToCore(motor_task, "motor", 3072, NULL, 8, NULL, 0);
}

void motor_control_submit(const motor_cmd_t *cmd) {
    xQueueSend(s_q, cmd, 0);
}

void motor_control_submit_override(const motor_cmd_t *cmd) {
    // Override jumps to front.
    xQueueSendToFront(s_q, cmd, 0);
}
