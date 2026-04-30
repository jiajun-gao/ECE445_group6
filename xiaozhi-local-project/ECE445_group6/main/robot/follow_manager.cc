#include "follow_manager.h"

#include "esp_log.h"

namespace robot {

namespace {
constexpr const char* TAG = "follow";

// --- Control parameters -----------------------------------------------------
// The spec was written assuming an analog IR that returns distance in cm
// (Kp * error proportional control, target 25 cm, etc.). Our board has a
// digital TCRT5000 DO, so the control loop degenerates to:
//
//   hand_present == true   -> hold constant slow forward speed
//   hand_present == false  -> stop
//
// All TIMING thresholds from the spec are honored verbatim below so the
// feel of the mode matches the description. If an analog sensor is added,
// swap hand_present for a distance reading and replace the "fixed forward
// speed" path with proportional control; the state machine stays the same.
constexpr uint8_t  FOLLOW_SPEED_PCT     = 25;     // slow indoor follow speed
constexpr uint32_t UPDATE_PERIOD_MS     = 40;     // control tick (spec: 30..50)
constexpr uint32_t STABLE_PRESENT_MS    = 300;    // spec: engage after 300 ms
constexpr uint32_t LOST_CONFIRM_MS      = 700;    // spec: lost after 700 ms
constexpr uint32_t WAIT_HAND_TIMEOUT_MS = 8000;   // spec: 8 s then give up
constexpr uint32_t LOST_TIMEOUT_MS      = 5000;   // spec: 5 s then IDLE
constexpr uint32_t WATCHDOG_MS          = 200;    // spec: motor watchdog

inline uint32_t NowMs() {
    return static_cast<uint32_t>(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

}  // namespace

FollowManager::~FollowManager() {
    Stop();
}

bool FollowManager::Init(MotorControl* motors, IrManager* ir, MoodCallback mood_cb) {
    if (motors == nullptr || ir == nullptr) return false;
    motors_  = motors;
    ir_      = ir;
    mood_cb_ = mood_cb;
    ESP_LOGI(TAG, "init ok (digital-IR follow variant)");
    return true;
}

bool FollowManager::Start() {
    if (motors_ == nullptr || ir_ == nullptr) {
        ESP_LOGE(TAG, "Start before Init");
        return false;
    }
    if (running_.exchange(true)) {
        ESP_LOGW(TAG, "already running; ignoring Start");
        return false;
    }
    abort_req_.store(false);
    phase_.store(FollowPhase::kWaitHand);

    BaseType_t ok = xTaskCreatePinnedToCore(
        &FollowManager::TaskEntry, "follow", 3 * 1024, this,
        tskIDLE_PRIORITY + 4, &task_, 1);
    if (ok != pdPASS) {
        ESP_LOGE(TAG, "xTaskCreate failed");
        running_.store(false);
        return false;
    }
    ESP_LOGI(TAG, "follow started (enter WAIT_HAND)");
    return true;
}

void FollowManager::Stop() {
    if (!running_.load()) return;
    abort_req_.store(true);
    ESP_LOGI(TAG, "stop requested");
}

void FollowManager::PublishMood(FollowMood m) {
    if (mood_cb_) mood_cb_(m);
}

void FollowManager::TaskEntry(void* arg) {
    static_cast<FollowManager*>(arg)->TaskLoop();
}

// ---------------------------------------------------------------------------
// Control loop.
//
// Tick cadence: 40 ms. Every tick we:
//   1. Read IR (front_obstacle + bottom_floor).
//   2. Abort on cliff (bottom_floor == false) regardless of phase.
//   3. Update hand-present / hand-absent streak timers.
//   4. Run the current phase (WAIT_HAND / ACTIVE / LOST).
//   5. Drive motors or brake, refresh last-motor-update timestamp.
//   6. Motor watchdog: if last update is older than WATCHDOG_MS, brake.
//
// Exit conditions (either drains into a normal "brake + clear face + IDLE"):
//   - abort_req_ set (Stop() from outside, or server follow_stop)
//   - WAIT_HAND timeout (8 s)
//   - LOST timeout (5 s)
//   - Cliff detected on the bottom IR
// ---------------------------------------------------------------------------
void FollowManager::TaskLoop() {
    const TickType_t period = pdMS_TO_TICKS(UPDATE_PERIOD_MS);
    TickType_t last = xTaskGetTickCount();

    uint32_t present_streak_ms    = 0;
    uint32_t absent_streak_ms     = 0;
    uint32_t wait_elapsed_ms      = 0;
    uint32_t lost_elapsed_ms      = 0;
    uint32_t last_motor_update_ms = NowMs();
    bool     terminate            = false;

    PublishMood(FollowMood::kWaitHand);
    ESP_LOGI(TAG, "phase=WAIT_HAND");

    while (!abort_req_.load() && !terminate) {
        TofReading r = ir_->Latest();
        const bool cliff = !r.bottom_floor;
        if (cliff) {
            ESP_LOGW(TAG, "cliff detected -> abort follow");
            break;
        }
        const bool hand = r.front_obstacle;  // digital proxy for "hand in range"

        if (hand) {
            present_streak_ms += UPDATE_PERIOD_MS;
            absent_streak_ms   = 0;
        } else {
            absent_streak_ms  += UPDATE_PERIOD_MS;
            present_streak_ms  = 0;
        }

        switch (phase_.load()) {
            case FollowPhase::kWaitHand: {
                wait_elapsed_ms += UPDATE_PERIOD_MS;
                if (present_streak_ms >= STABLE_PRESENT_MS) {
                    SetPhase(FollowPhase::kActive);
                    PublishMood(FollowMood::kActive);
                    wait_elapsed_ms = 0;
                    ESP_LOGI(TAG, "hand stable %ums -> ACTIVE",
                             (unsigned)STABLE_PRESENT_MS);
                } else if (wait_elapsed_ms >= WAIT_HAND_TIMEOUT_MS) {
                    ESP_LOGI(TAG, "WAIT_HAND timed out; exiting follow");
                    terminate = true;
                    break;
                }
                motors_->Stop(false);
                last_motor_update_ms = NowMs();
                break;
            }

            case FollowPhase::kActive: {
                if (absent_streak_ms >= LOST_CONFIRM_MS) {
                    SetPhase(FollowPhase::kLost);
                    PublishMood(FollowMood::kLost);
                    motors_->Stop(true);
                    lost_elapsed_ms = 0;
                    last_motor_update_ms = NowMs();
                    ESP_LOGI(TAG, "hand absent %ums -> LOST",
                             (unsigned)LOST_CONFIRM_MS);
                    break;
                }
                // Digital variant: constant slow forward while hand_present.
                // Analog variant would compute base_speed = clamp(Kp*error).
                motors_->SetLeft (static_cast<int8_t>(FOLLOW_SPEED_PCT));
                motors_->SetRight(static_cast<int8_t>(FOLLOW_SPEED_PCT));
                last_motor_update_ms = NowMs();
                break;
            }

            case FollowPhase::kLost: {
                lost_elapsed_ms += UPDATE_PERIOD_MS;
                if (present_streak_ms >= STABLE_PRESENT_MS) {
                    SetPhase(FollowPhase::kActive);
                    PublishMood(FollowMood::kActive);
                    lost_elapsed_ms = 0;
                    ESP_LOGI(TAG, "hand returned -> ACTIVE");
                    break;
                }
                if (lost_elapsed_ms >= LOST_TIMEOUT_MS) {
                    ESP_LOGI(TAG, "LOST timed out; exiting follow");
                    terminate = true;
                    break;
                }
                motors_->Stop(true);
                last_motor_update_ms = NowMs();
                break;
            }

            case FollowPhase::kIdle:
            default:
                terminate = true;
                break;
        }

        // Motor watchdog — belt-and-braces. Loop runs every 40 ms so this
        // should never fire in practice, but if any phase skipped an update
        // we fail safe to "brake".
        if (NowMs() - last_motor_update_ms > WATCHDOG_MS) {
            motors_->Stop(true);
            last_motor_update_ms = NowMs();
            ESP_LOGW(TAG, "motor watchdog -> brake");
        }

        vTaskDelayUntil(&last, period);
    }

    motors_->Stop(true);
    PublishMood(FollowMood::kNone);
    SetPhase(FollowPhase::kIdle);
    running_.store(false);
    abort_req_.store(false);
    task_ = nullptr;

    ESP_LOGI(TAG, "follow task exited");
    vTaskDelete(nullptr);
}

}  // namespace robot
