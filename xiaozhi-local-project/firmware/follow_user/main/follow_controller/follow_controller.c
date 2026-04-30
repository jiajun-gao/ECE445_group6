#include "follow_controller.h"
#include "config.h"
#include "motor_control.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include <math.h>
#include <string.h>

static const char *TAG = "follow";

static tracker_t        s_tr;
static SemaphoreHandle_t s_mu;
static volatile bool    s_enabled;
static volatile follow_state_t s_state = FOLLOW_IDLE;
static int8_t           s_last_seen_side = 0; // for SEARCH direction

static int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

static void emit(motor_verb_t v, int pwm) {
    motor_cmd_t c = { .verb = v, .pwm = clampi(pwm, 0, 255), .source = CMD_SRC_FOLLOW };
    motor_control_submit(&c);
}

static void decide(void) {
    float err, size;
    bool have = tracker_get_error(&s_tr, &err, &size);

    // ---- Lost-target handling ----
    if (!have || s_tr.lost_frames > 0) {
        if (s_tr.lost_frames >= LOST_GIVEUP_FRAMES) {
            s_state = FOLLOW_IDLE;
            emit(CMD_STOP, 0);
            return;
        }
        if (s_tr.lost_frames >= LOST_SEARCH_FRAMES) {
            s_state = FOLLOW_SEARCHING;
            // slow spin toward last-seen side
            if (s_last_seen_side >= 0) emit(CMD_SPIN_RIGHT, 110);
            else                       emit(CMD_SPIN_LEFT,  110);
            return;
        }
        if (s_tr.lost_frames >= LOST_STOP_FRAMES) {
            emit(CMD_STOP, 0);
            return;
        }
        // brief glitch: keep doing nothing new, motor will hold last cmd
        return;
    }

    // ---- Have a target ----
    s_state = FOLLOW_TRACKING;
    s_last_seen_side = (err < 0) ? -1 : +1;

    float abs_err = fabsf(err);

    // Hysteretic center band
    bool in_center;
    if (s_tr.centered) in_center = (abs_err < DEAD_ZONE_EXIT);
    else               in_center = (abs_err < DEAD_ZONE);
    s_tr.centered = in_center;

    if (!in_center) {
        // turn or spin
        if (abs_err > SOFT_TURN) {
            int pwm = clampi((int)(KP_TURN * abs_err), 120, MAX_TURN_PWM);
            if (err < 0) emit(CMD_SPIN_LEFT,  pwm);
            else         emit(CMD_SPIN_RIGHT, pwm);
        } else {
            int pwm = clampi((int)(KP_TURN * abs_err), 100, MAX_TURN_PWM);
            if (err < 0) emit(CMD_LEFT,  pwm);
            else         emit(CMD_RIGHT, pwm);
        }
        return;
    }

    // Centered — distance decides
    if (size < FAR_RATIO) {
        int pwm = clampi((int)(KF_FWD * (FAR_RATIO - size)) + 100, 100, MAX_FWD_PWM);
        emit(CMD_FORWARD, pwm);
    } else if (size > NEAR_RATIO) {
        // too close — hold (or small backoff if you want)
        s_state = FOLLOW_HOLD;
        emit(CMD_STOP, 0);
    } else {
        s_state = FOLLOW_HOLD;
        emit(CMD_STOP, 0);
    }
}

static void control_task(void *arg) {
    const TickType_t period = pdMS_TO_TICKS(50); // 20 Hz
    for (;;) {
        if (s_enabled) {
            xSemaphoreTake(s_mu, portMAX_DELAY);
            decide();
            xSemaphoreGive(s_mu);
        }
        vTaskDelay(period);
    }
}

void follow_init(void) {
    tracker_init(&s_tr);
    s_mu = xSemaphoreCreateMutex();
    s_enabled = false;
}

void follow_start(void) {
    xTaskCreatePinnedToCore(control_task, "follow", 4096, NULL, 7, NULL, 0);
}

void follow_feed_target(const target_t *det) {
    xSemaphoreTake(s_mu, portMAX_DELAY);
    tracker_update(&s_tr, det);
    xSemaphoreGive(s_mu);
}

void follow_enable(bool on) {
    s_enabled = on;
    if (!on) {
        motor_cmd_t c = { .verb = CMD_STOP, .pwm = 0, .source = CMD_SRC_FOLLOW };
        motor_control_submit(&c);
        s_state = FOLLOW_IDLE;
    }
    ESP_LOGI(TAG, "enable=%d", on);
}

follow_state_t follow_get_state(void) { return s_state; }
