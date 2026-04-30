#ifndef ROBOT_MOTOR_CONTROL_H_
#define ROBOT_MOTOR_CONTROL_H_

#include <stdint.h>
#include "driver/gpio.h"

namespace robot {

// Motor pin map per schematic.
// NOTE: GPIO3 is a strapping pin on ESP32-S3 (JTAG source). The code tolerates
// it, but verify at power-up that the driver input does not pull it low.
constexpr gpio_num_t MOTOR1_IN_A_GPIO = GPIO_NUM_17;  // Left motor, direction A
constexpr gpio_num_t MOTOR1_IN_B_GPIO = GPIO_NUM_18;  // Left motor, direction B
constexpr gpio_num_t MOTOR2_IN_A_GPIO = GPIO_NUM_8;   // Right motor, direction A
constexpr gpio_num_t MOTOR2_IN_B_GPIO = GPIO_NUM_3;   // Right motor, direction B

// LEDC PWM configuration.
// 20 kHz keeps audible whine out of the mic path; 10-bit = 1024 steps duty.
constexpr uint32_t MOTOR_PWM_FREQ_HZ  = 20000;
constexpr uint8_t  MOTOR_PWM_RES_BITS = 10;
constexpr uint16_t MOTOR_PWM_DUTY_MAX = (1u << MOTOR_PWM_RES_BITS) - 1;

// Per-motor signed speed: -100..+100 (% duty, sign selects direction).
// 0 performs coast (both inputs low). Use Brake() for active brake.
class MotorControl {
public:
    bool Init();

    // speed_percent: -100..+100.  Positive = forward for that motor.
    void SetLeft(int8_t speed_percent);
    void SetRight(int8_t speed_percent);

    // Stops both motors. mode=false coasts (IN_A=IN_B=0), mode=true brakes
    // (IN_A=IN_B=1, high-side short on H-bridge drivers such as DRV8833/TB6612).
    void Stop(bool active_brake = false);

private:
    void ApplyMotor(uint8_t channel_a, uint8_t channel_b, int8_t speed_percent);
    bool initialized_ = false;
};

}  // namespace robot

#endif  // ROBOT_MOTOR_CONTROL_H_
