#include "dance_manager.h"

#include <algorithm>
#include <array>

#include "esp_log.h"

namespace robot {

namespace {
constexpr const char* TAG = "dance";

// Dance preset — roughly 16 seconds, grouped into four expressive phases.
// Keep step durations ≤ 500 ms each so abort latency stays small; long
// "holds" are built up from multiple short identical steps. Speeds are
// deliberately under 90 so a light desk bot doesn't roll itself off the
// table even if an IR safety check briefly drops out mid-dance.
//
//   Phase 1 — INTRO  (~2.5 s)  : gentle rock + "ready" pose
//   Phase 2 — MAIN   (~7.0 s)  : left/right/forward/back rhythm combos
//   Phase 3 — HIGH   (~3.5 s)  : fast spin climax with vibrato reversals
//   Phase 4 — END    (~2.0 s)  : full stop + big happy face
//
// Total ≈ 15–16 s. DanceManager::PresetDurationMs() returns the exact sum
// so the server can schedule the "how did I do?" prompt right after.
constexpr DanceStep kDance[] = {
    // --- Phase 1: INTRO, little side-to-side rock ----------------------------
    {   0,    0,  200, DanceMood::kReady      },     // pose
    {  35,  -35,  250, DanceMood::kReady      },     // rock right (slow)
    { -35,   35,  250, DanceMood::kReady      },     // rock left
    {  45,  -45,  250, DanceMood::kReady      },     // rock right again (bigger)
    { -45,   45,  250, DanceMood::kReady      },     // rock left
    {   0,    0,  150, DanceMood::kReady      },     // settle, anticipation
    // Nominal: 1350 ms

    // --- Phase 2: MAIN, a playful L/R groove + step-forward/back -------------
    {  70,   70,  300, DanceMood::kExcited    },     // quick forward
    {   0,    0,  100, DanceMood::kExcited    },     // hitch
    { -70,  -70,  300, DanceMood::kExcited    },     // quick back
    {   0,    0,  100, DanceMood::kExcited    },     // hitch

    {  80,    0,  250, DanceMood::kExcited    },     // arc right
    {   0,   80,  250, DanceMood::kExcited    },     // arc left
    {  80,    0,  250, DanceMood::kExcited    },     // arc right
    {   0,   80,  250, DanceMood::kExcited    },     // arc left
    {   0,    0,  120, DanceMood::kExcited    },     // beat pause

    {  60,  -60,  400, DanceMood::kSpin       },     // short spin right
    { -60,   60,  400, DanceMood::kSpin       },     // short spin left
    {   0,    0,  150, DanceMood::kExcited    },     // breath

    {  85,   85,  220, DanceMood::kExcited    },     // forward pop
    { -85,  -85,  220, DanceMood::kExcited    },     // back pop
    {   0,    0,  150, DanceMood::kExcited    },     // pause
    // Nominal: ~3880 ms (phases 1+2 ≈ 5.2 s)

    // --- Phase 3: HIGHLIGHT — fast spin climax with quick reversals ----------
    {  90,  -90,  350, DanceMood::kHighlight  },     // fast spin right
    {  90,  -90,  350, DanceMood::kHighlight  },
    {  90,  -90,  350, DanceMood::kHighlight  },     // 3x = ~1 full body rotation
    {   0,    0,   80, DanceMood::kHighlight  },     // snap!
    { -90,   90,  350, DanceMood::kHighlight  },     // reverse spin
    { -90,   90,  350, DanceMood::kHighlight  },
    {   0,    0,  120, DanceMood::kHighlight  },     // snap!
    {  90,  -90,  300, DanceMood::kHighlight  },     // one more right snap
    {   0,    0,  150, DanceMood::kHighlight  },     // freeze
    // Nominal: ~2500 ms

    // --- Phase 4: END POSE — full stop + big happy smile --------------------
    {   0,    0,  400, DanceMood::kEndPose    },
    {  20,  -20,  200, DanceMood::kEndPose    },     // little "ta-da" wiggle
    { -20,   20,  200, DanceMood::kEndPose    },
    {   0,    0,  900, DanceMood::kEndPose    },     // hold smile
    // Nominal: 1700 ms

    // Sequence length cap: 28 steps (fits easily in stack/array).
};

constexpr size_t kDanceSteps = sizeof(kDance) / sizeof(kDance[0]);

uint32_t SumDurationsMs() {
    uint32_t total = 0;
    for (size_t i = 0; i < kDanceSteps; ++i) total += kDance[i].duration_ms;
    return total;
}

}  // namespace

DanceManager::~DanceManager() {
    Stop();
    // TaskLoop exits on abort; task handle is not explicitly deleted here
    // because Stop() joins (TaskLoop calls vTaskDelete(nullptr) on exit).
}

uint32_t DanceManager::PresetDurationMs() {
    return SumDurationsMs();
}

bool DanceManager::Init(MotorControl* motors, MoodCallback mood_cb) {
    if (motors == nullptr) return false;
    motors_  = motors;
    mood_cb_ = mood_cb;
    ESP_LOGI(TAG, "init ok, preset %u steps / %lu ms",
             (unsigned)kDanceSteps, (unsigned long)SumDurationsMs());
    return true;
}

bool DanceManager::Start(uint32_t* out_duration_ms) {
    if (motors_ == nullptr) {
        ESP_LOGE(TAG, "Start before Init");
        return false;
    }
    if (running_.exchange(true)) {
        ESP_LOGW(TAG, "already running; ignoring Start");
        return false;
    }
    abort_req_.store(false);
    if (out_duration_ms) *out_duration_ms = SumDurationsMs();

    // Dedicated task so the dance doesn't piggy-back on the Wi-Fi / audio
    // pipelines. Stack is tight (3 KB) because this function is basically
    // "write a couple of LEDC duty cycles and sleep".
    BaseType_t ok = xTaskCreatePinnedToCore(
        &DanceManager::TaskEntry, "dance", 3 * 1024, this,
        tskIDLE_PRIORITY + 4, &task_, 1);
    if (ok != pdPASS) {
        ESP_LOGE(TAG, "xTaskCreate failed");
        running_.store(false);
        return false;
    }
    ESP_LOGI(TAG, "dance started (~%lu ms)", (unsigned long)SumDurationsMs());
    return true;
}

void DanceManager::Stop() {
    if (!running_.load()) return;
    abort_req_.store(true);
    // TaskLoop polls abort_req_ between steps and on every 20 ms tick.
    ESP_LOGI(TAG, "stop requested");
}

void DanceManager::TaskEntry(void* arg) {
    static_cast<DanceManager*>(arg)->TaskLoop();
}

void DanceManager::TaskLoop() {
    ESP_LOGI(TAG, "task running");

    // Soft ramp limits: max Δduty between consecutive 20 ms ticks. Keeps
    // current inrush reasonable on a small LiPo so Wi-Fi TX doesn't drop
    // out mid-dance when the motors suddenly reverse.
    constexpr int kMaxDeltaPerTick   = 25;   // percent per 20 ms
    constexpr TickType_t kTickMs     = 20;
    constexpr TickType_t kTickDelay  = pdMS_TO_TICKS(kTickMs);

    int8_t cur_l = 0, cur_r = 0;

    auto ramp_to = [&](int8_t tgt_l, int8_t tgt_r, uint16_t total_ms) {
        // Produce ceil(total_ms / kTickMs) ticks; on each tick push duty
        // cycle kMaxDeltaPerTick closer to the target, then sleep. The
        // last tick lands exactly on the target.
        uint16_t remaining = total_ms;
        while (remaining > 0 && !abort_req_.load()) {
            int dl = tgt_l - cur_l;
            int dr = tgt_r - cur_r;
            if (dl >  kMaxDeltaPerTick) dl =  kMaxDeltaPerTick;
            if (dl < -kMaxDeltaPerTick) dl = -kMaxDeltaPerTick;
            if (dr >  kMaxDeltaPerTick) dr =  kMaxDeltaPerTick;
            if (dr < -kMaxDeltaPerTick) dr = -kMaxDeltaPerTick;
            cur_l = static_cast<int8_t>(cur_l + dl);
            cur_r = static_cast<int8_t>(cur_r + dr);
            motors_->SetLeft(cur_l);
            motors_->SetRight(cur_r);
            TickType_t dt = (remaining >= kTickMs) ? kTickDelay
                                                   : pdMS_TO_TICKS(remaining);
            vTaskDelay(dt);
            remaining = (remaining > kTickMs) ? (remaining - kTickMs) : 0;
        }
    };

    for (size_t i = 0; i < kDanceSteps; ++i) {
        if (abort_req_.load()) break;
        const DanceStep& s = kDance[i];

        if (mood_cb_) mood_cb_(s.mood);
        ESP_LOGD(TAG, "step %u: L=%d R=%d dur=%u",
                 (unsigned)i, s.left_speed, s.right_speed, s.duration_ms);

        ramp_to(s.left_speed, s.right_speed, s.duration_ms);
    }

    // Always leave the robot safely stopped and the face cleared.
    motors_->Stop(/*active_brake=*/true);
    if (mood_cb_) mood_cb_(DanceMood::kNone);

    ESP_LOGI(TAG, "task done (aborted=%d)", abort_req_.load());
    running_.store(false);
    abort_req_.store(false);
    task_ = nullptr;
    vTaskDelete(nullptr);
}

}  // namespace robot
