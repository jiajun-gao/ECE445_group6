#include "motor_control.h"

#include <algorithm>
#include "driver/ledc.h"
#include "esp_log.h"

namespace robot {

namespace {
constexpr const char* TAG = "motor_ctrl";

// Each IN pin gets its own LEDC channel so sign-magnitude drive works:
//   forward  -> IN_A = PWM(duty), IN_B = 0
//   backward -> IN_A = 0,         IN_B = PWM(duty)
//   brake    -> IN_A = max,       IN_B = max
//   coast    -> IN_A = 0,         IN_B = 0
constexpr ledc_timer_t       PWM_TIMER  = LEDC_TIMER_0;
constexpr ledc_mode_t        PWM_MODE   = LEDC_LOW_SPEED_MODE;

constexpr ledc_channel_t M1A_CH = LEDC_CHANNEL_0;
constexpr ledc_channel_t M1B_CH = LEDC_CHANNEL_1;
constexpr ledc_channel_t M2A_CH = LEDC_CHANNEL_2;
constexpr ledc_channel_t M2B_CH = LEDC_CHANNEL_3;

inline uint32_t DutyFromPercent(uint8_t pct) {
    if (pct > 100) pct = 100;
    return (static_cast<uint32_t>(MOTOR_PWM_DUTY_MAX) * pct) / 100u;
}

esp_err_t ConfigChannel(ledc_channel_t ch, gpio_num_t gpio) {
    ledc_channel_config_t cfg = {};
    cfg.gpio_num   = gpio;
    cfg.speed_mode = PWM_MODE;
    cfg.channel    = ch;
    cfg.intr_type  = LEDC_INTR_DISABLE;
    cfg.timer_sel  = PWM_TIMER;
    cfg.duty       = 0;
    cfg.hpoint     = 0;
    return ledc_channel_config(&cfg);
}

void WriteDuty(ledc_channel_t ch, uint32_t duty) {
    ledc_set_duty(PWM_MODE, ch, duty);
    ledc_update_duty(PWM_MODE, ch);
}
}  // namespace

bool MotorControl::Init() {
    if (initialized_) return true;

    ledc_timer_config_t tcfg = {};
    tcfg.speed_mode      = PWM_MODE;
    tcfg.duty_resolution = static_cast<ledc_timer_bit_t>(MOTOR_PWM_RES_BITS);
    tcfg.timer_num       = PWM_TIMER;
    tcfg.freq_hz         = MOTOR_PWM_FREQ_HZ;
    tcfg.clk_cfg         = LEDC_AUTO_CLK;
    if (ledc_timer_config(&tcfg) != ESP_OK) {
        ESP_LOGE(TAG, "ledc_timer_config failed");
        return false;
    }

    if (ConfigChannel(M1A_CH, MOTOR1_IN_A_GPIO) != ESP_OK ||
        ConfigChannel(M1B_CH, MOTOR1_IN_B_GPIO) != ESP_OK ||
        ConfigChannel(M2A_CH, MOTOR2_IN_A_GPIO) != ESP_OK ||
        ConfigChannel(M2B_CH, MOTOR2_IN_B_GPIO) != ESP_OK) {
        ESP_LOGE(TAG, "ledc_channel_config failed");
        return false;
    }

    Stop(false);
    initialized_ = true;
    ESP_LOGI(TAG, "Motor PWM ready: %lu Hz, %u-bit", MOTOR_PWM_FREQ_HZ, MOTOR_PWM_RES_BITS);
    return true;
}

void MotorControl::ApplyMotor(uint8_t ch_a, uint8_t ch_b, int8_t speed_percent) {
    speed_percent = std::max<int8_t>(-100, std::min<int8_t>(100, speed_percent));
    if (speed_percent == 0) {
        WriteDuty(static_cast<ledc_channel_t>(ch_a), 0);
        WriteDuty(static_cast<ledc_channel_t>(ch_b), 0);
        return;
    }
    uint32_t duty = DutyFromPercent(static_cast<uint8_t>(
        speed_percent > 0 ? speed_percent : -speed_percent));
    if (speed_percent > 0) {
        WriteDuty(static_cast<ledc_channel_t>(ch_a), duty);
        WriteDuty(static_cast<ledc_channel_t>(ch_b), 0);
    } else {
        WriteDuty(static_cast<ledc_channel_t>(ch_a), 0);
        WriteDuty(static_cast<ledc_channel_t>(ch_b), duty);
    }
}

void MotorControl::SetLeft(int8_t speed_percent) {
    ApplyMotor(M1A_CH, M1B_CH, speed_percent);
}

void MotorControl::SetRight(int8_t speed_percent) {
    ApplyMotor(M2A_CH, M2B_CH, speed_percent);
}

void MotorControl::Stop(bool active_brake) {
    uint32_t v = active_brake ? MOTOR_PWM_DUTY_MAX : 0;
    WriteDuty(M1A_CH, v);
    WriteDuty(M1B_CH, v);
    WriteDuty(M2A_CH, v);
    WriteDuty(M2B_CH, v);
}

}  // namespace robot
