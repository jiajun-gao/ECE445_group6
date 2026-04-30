#include "safety_manager.h"
#include "config.h"
#include "motor_control.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "safety";

static int s_front_gpio = -1;
static int s_edge_gpio  = -1;
static volatile safety_state_t s_state;
static volatile bool s_override;

static inline bool ir_active(int gpio, bool active_low) {
    int v = gpio_get_level(gpio);
    return active_low ? (v == 0) : (v == 1);
}

static void safety_task(void *arg) {
    const TickType_t period = pdMS_TO_TICKS(1000 / SAFETY_TASK_HZ);
    bool latched = false;
    uint32_t clear_ticks = 0;

    for (;;) {
        safety_state_t s = {
            .front_blocked = ir_active(s_front_gpio, FRONT_IR_ACTIVE_LOW),
            .edge_detected = ir_active(s_edge_gpio,  EDGE_IR_ACTIVE_LOW),
        };
        s_state = s;

        bool danger = s.front_blocked || s.edge_detected;
        if (danger) {
            if (!latched) {
                ESP_LOGW(TAG, "SAFETY TRIP front=%d edge=%d", s.front_blocked, s.edge_detected);
                motor_cmd_t stop = { .verb = CMD_STOP, .pwm = 0, .source = CMD_SRC_SAFETY };
                motor_control_submit_override(&stop);
                // On edge detection, nudge back briefly to pull away from the drop.
                if (s.edge_detected) {
                    motor_cmd_t back = { .verb = CMD_BACKWARD, .pwm = 140, .source = CMD_SRC_SAFETY };
                    motor_control_submit_override(&back);
                    vTaskDelay(pdMS_TO_TICKS(180));
                    motor_control_submit_override(&stop);
                }
                latched = true;
            }
            clear_ticks = 0;
            s_override = true;
        } else if (latched) {
            clear_ticks++;
            // require sensors clear for ~500 ms before releasing override
            if (clear_ticks > (SAFETY_TASK_HZ / 2)) {
                latched = false;
                s_override = false;
                ESP_LOGI(TAG, "SAFETY CLEAR");
            }
        }
        vTaskDelay(period);
    }
}

void safety_init(int front_ir_gpio, int edge_ir_gpio) {
    s_front_gpio = front_ir_gpio;
    s_edge_gpio  = edge_ir_gpio;
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << front_ir_gpio) | (1ULL << edge_ir_gpio),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&io);
    memset((void*)&s_state, 0, sizeof(s_state));
    s_override = false;
}

void safety_start(void) {
    // Core 0, high priority so it can preempt anything else.
    xTaskCreatePinnedToCore(safety_task, "safety", 4096, NULL, 10, NULL, 0);
}

safety_state_t safety_get(void) { return s_state; }
bool safety_override_active(void) { return s_override; }
