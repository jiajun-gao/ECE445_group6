#ifndef ROBOT_IR_MANAGER_H_
#define ROBOT_IR_MANAGER_H_

#include <atomic>
#include <stdint.h>
#include "driver/gpio.h"
#include "robot_types.h"

namespace robot {

// TCRT5000 DO (digital comparator output, threshold set by on-module pot).
//   Front sensor -> GPIO15, looks FORWARD for obstacles.
//   Bottom sensor -> GPIO16, looks DOWN at the desk for cliff detection.
//
// The two sensors detect OPPOSITE things:
//   Front: active when an obstacle reflects IR back  -> "obstacle" signal
//   Bottom: active when the desk reflects IR back    -> "floor present" signal
// Most TCRT5000 modules drive DO LOW on reflection (comparator trips below
// Vref). If yours inverts, flip the matching constant below.
constexpr gpio_num_t IR_FRONT_GPIO  = GPIO_NUM_15;
constexpr gpio_num_t IR_BOTTOM_GPIO = GPIO_NUM_16;

// "Active" = event detected by that sensor:
//   FRONT active -> obstacle present
//   BOTTOM active -> floor present
// Default assumption: DO goes LOW on reflection.
constexpr int FRONT_ACTIVE_LEVEL  = 0;  // 0=LOW means obstacle detected
constexpr int BOTTOM_ACTIVE_LEVEL = 0;  // 0=LOW means floor detected

// 500 Hz sampling, 5 consecutive samples = ~10 ms to accept a state change.
constexpr uint8_t IR_DEBOUNCE_HITS = 5;

class IrManager {
public:
    bool Init();
    bool StartTask();

    TofReading Latest() const;

private:
    static void TaskTrampoline(void* arg);
    void TaskLoop();

    std::atomic<bool> front_obstacle_{false};
    std::atomic<bool> bottom_floor_{true};
    std::atomic<uint8_t> classification_{0};  // SafetyReason
    bool initialized_ = false;
};

}  // namespace robot

#endif  // ROBOT_IR_MANAGER_H_
