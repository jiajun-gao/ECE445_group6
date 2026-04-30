#include "face_display.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <esp_log.h>
#include <esp_random.h>

#include "assets/lang_config.h"
#include "mcp_server.h"

#define TAG "FaceLcdDisplay"

namespace {

// --- Layout constants for the 160x80 ST7735 -------------------------------
// Kept as a single block so the face can be re-tuned in one place. All
// coordinates are in LVGL screen pixels; we re-read width()/height() at
// runtime in case a future board reuses this class at a different size.
constexpr int kTransitionMs        = 300;
constexpr int kAnimTickMs          = 33;    // ~30 FPS
constexpr int kIdleBlinkMinMs      = 4000;
constexpr int kIdleBlinkMaxMs      = 6000;
constexpr int kListenBlinkMinMs    = 2000;
constexpr int kListenBlinkMaxMs    = 4000;
constexpr int kBlinkCloseMs        = 120;
constexpr int kErrorBlinkPeriodMs  = 320;   // ~3 Hz on/off flash

// Black on white, as agreed in the design conversation. Kept as constants so
// switching to a dark theme later is a two-line edit.
const lv_color_t kBgColor  = lv_color_white();
const lv_color_t kFgColor  = lv_color_black();

// Simple smoothstep: t in [0,1] -> eased in/out.
inline float Smoothstep(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

inline int Lerp(int a, int b, float t) {
    return a + static_cast<int>((b - a) * t + (t >= 0 ? 0.5f : -0.5f));
}
inline float Lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

// Error-emotion aliases. Server-side LLM may emit any of these when the
// chat flow hits a failure; we map all of them to the single kError face.
bool IsErrorEmotion(const char* e) {
    if (e == nullptr) return false;
    return std::strcmp(e, "circle_xmark") == 0 ||
           std::strcmp(e, "triangle_exclamation") == 0 ||
           std::strcmp(e, "error") == 0 ||
           std::strcmp(e, "confused") == 0 ||
           std::strcmp(e, "frustrated") == 0;
}

}  // namespace

FaceLcdDisplay::FaceLcdDisplay(esp_lcd_panel_io_handle_t panel_io,
                               esp_lcd_panel_handle_t panel,
                               int width, int height,
                               int offset_x, int offset_y,
                               bool mirror_x, bool mirror_y, bool swap_xy)
    : SpiLcdDisplay(panel_io, panel, width, height, offset_x, offset_y,
                    mirror_x, mirror_y, swap_xy) {}

FaceLcdDisplay::~FaceLcdDisplay() {
    if (anim_timer_ != nullptr) {
        DisplayLockGuard lock(this);
        lv_timer_del(anim_timer_);
        anim_timer_ = nullptr;
    }
}

// ---------------------------------------------------------------------------
// SetupUI: let the parent build its widget tree (so SetChatMessage / theme
// switching / low-battery popup keep working), then HIDE everything that
// would paint pixels and overlay a full-screen white face canvas of our own.
// ---------------------------------------------------------------------------
void FaceLcdDisplay::SetupUI() {
    SpiLcdDisplay::SetupUI();

    DisplayLockGuard lock(this);

    // Globally suppress subtitles (parent would otherwise un-hide bottom_bar_
    // whenever an assistant message arrives, painting text over our face).
    SetHideSubtitle(true);

    // Hide every widget the parent drew on lv_screen_active() so our face
    // owns the entire panel.
    for (auto* w : { container_, status_bar_, emoji_image_, emoji_label_,
                     low_battery_popup_ }) {
        if (w != nullptr) lv_obj_add_flag(w, LV_OBJ_FLAG_HIDDEN);
    }

    BuildFaceWidgets();

    // 30 FPS render pump. lv_timer callbacks run on the LVGL task, which
    // already holds the port lock, so AnimTick() does not need its own lock.
    anim_timer_ = lv_timer_create(&FaceLcdDisplay::AnimTickThunk,
                                  kAnimTickMs, this);

    state_enter_tick_   = lv_tick_get();
    transition_start_   = state_enter_tick_ - kTransitionMs;  // skip initial fade-in
    next_blink_tick_    = state_enter_tick_ + kIdleBlinkMinMs;

    // Seed with idle pose so there's something on screen before the first tick.
    ApplyPose(ParamsForState(FaceState::kIdle), 0, 0, 0, 0, -1);
}

// ---------------------------------------------------------------------------
void FaceLcdDisplay::BuildFaceWidgets() {
    auto* screen = lv_screen_active();

    face_root_ = lv_obj_create(screen);
    lv_obj_set_size(face_root_, width(), height());
    lv_obj_set_pos(face_root_, 0, 0);
    lv_obj_set_style_radius(face_root_, 0, 0);
    lv_obj_set_style_pad_all(face_root_, 0, 0);
    lv_obj_set_style_border_width(face_root_, 0, 0);
    lv_obj_set_style_bg_color(face_root_, kBgColor, 0);
    lv_obj_set_style_bg_opa(face_root_, LV_OPA_COVER, 0);
    lv_obj_clear_flag(face_root_, LV_OBJ_FLAG_SCROLLABLE);

    // Eye bodies: rounded rect with max radius, so any w/h renders as a
    // pill/circle/line.
    for (int i = 0; i < 2; ++i) {
        eye_body_[i] = lv_obj_create(face_root_);
        lv_obj_set_style_radius(eye_body_[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(eye_body_[i], kFgColor, 0);
        lv_obj_set_style_bg_opa(eye_body_[i], LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(eye_body_[i], 0, 0);
        lv_obj_set_style_pad_all(eye_body_[i], 0, 0);
        lv_obj_clear_flag(eye_body_[i], LV_OBJ_FLAG_SCROLLABLE);
    }

    // Smile-eye overlays: top-half arc (inverted U), drawn as a black line
    // on white. We always keep them parented and alive; visibility is
    // animated purely through opacity so state transitions cross-fade.
    for (int i = 0; i < 2; ++i) {
        eye_arc_[i] = lv_arc_create(face_root_);
        lv_obj_set_size(eye_arc_[i], 30, 18);
        lv_arc_set_rotation(eye_arc_[i], 0);
        lv_arc_set_bg_angles(eye_arc_[i], 0, 360);
        lv_arc_set_angles(eye_arc_[i], 180, 360);  // top half (∩)
        lv_obj_set_style_bg_opa(eye_arc_[i], LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(eye_arc_[i], 0, 0);
        lv_obj_set_style_pad_all(eye_arc_[i], 0, 0);
        lv_obj_set_style_arc_opa(eye_arc_[i], LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_arc_color(eye_arc_[i], kFgColor, LV_PART_INDICATOR);
        lv_obj_set_style_arc_width(eye_arc_[i], 4, LV_PART_INDICATOR);
        lv_obj_remove_style(eye_arc_[i], nullptr, LV_PART_KNOB);
        lv_obj_clear_flag(eye_arc_[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_opa(eye_arc_[i], LV_OPA_TRANSP, 0);
    }

    // Mouth rectangle (line / open-mouth pill).
    mouth_rect_ = lv_obj_create(face_root_);
    lv_obj_set_style_radius(mouth_rect_, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(mouth_rect_, kFgColor, 0);
    lv_obj_set_style_border_width(mouth_rect_, 0, 0);
    lv_obj_set_style_pad_all(mouth_rect_, 0, 0);
    lv_obj_clear_flag(mouth_rect_, LV_OBJ_FLAG_SCROLLABLE);

    // Mouth smile arc (bottom half, ⌣).
    mouth_arc_ = lv_arc_create(face_root_);
    lv_obj_set_size(mouth_arc_, 40, 20);
    lv_arc_set_rotation(mouth_arc_, 0);
    lv_arc_set_bg_angles(mouth_arc_, 0, 360);
    lv_arc_set_angles(mouth_arc_, 0, 180);  // bottom half
    lv_obj_set_style_bg_opa(mouth_arc_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(mouth_arc_, 0, 0);
    lv_obj_set_style_pad_all(mouth_arc_, 0, 0);
    lv_obj_set_style_arc_opa(mouth_arc_, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_arc_color(mouth_arc_, kFgColor, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(mouth_arc_, 4, LV_PART_INDICATOR);
    lv_obj_remove_style(mouth_arc_, nullptr, LV_PART_KNOB);
    lv_obj_clear_flag(mouth_arc_, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_opa(mouth_arc_, LV_OPA_TRANSP, 0);

    // Thinking dots -- small dark circles, visibility driven by AnimTick.
    for (int i = 0; i < 3; ++i) {
        dot_[i] = lv_obj_create(face_root_);
        lv_obj_set_size(dot_[i], 5, 5);
        lv_obj_set_style_radius(dot_[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(dot_[i], kFgColor, 0);
        lv_obj_set_style_border_width(dot_[i], 0, 0);
        lv_obj_set_style_pad_all(dot_[i], 0, 0);
        lv_obj_add_flag(dot_[i], LV_OBJ_FLAG_HIDDEN);
    }
}

// ---------------------------------------------------------------------------
// State setters. Called from Application / robot / MCP threads; they just
// flip atomics and let the LVGL timer pick up the change on the next tick.
// ---------------------------------------------------------------------------
void FaceLcdDisplay::SetStatus(const char* status) {
    // NOTE: deliberately NOT calling SpiLcdDisplay::SetStatus here -- parent
    // would write into status_label_, which we hid. The server still logs
    // the transition, so no observability is lost.
    if (status == nullptr) return;

    FaceState next = voice_state_.load();
    if (std::strcmp(status, Lang::Strings::LISTENING)  == 0) next = FaceState::kListening;
    else if (std::strcmp(status, Lang::Strings::SPEAKING)   == 0) next = FaceState::kSpeaking;
    else if (std::strcmp(status, Lang::Strings::CONNECTING) == 0) next = FaceState::kThinking;
    else if (std::strcmp(status, Lang::Strings::STANDBY)    == 0) next = FaceState::kIdle;
    voice_state_.store(next);
}

void FaceLcdDisplay::SetEmotion(const char* emotion) {
    // Two jobs: (1) latch the error state when the server sends an error
    // emotion; (2) clear it again on any non-error emotion tag.
    bool err = IsErrorEmotion(emotion);
    errored_.store(err);
    ESP_LOGI(TAG, "SetEmotion('%s') -> %s", emotion ? emotion : "(null)",
             err ? "ERROR" : "ok");
}

void FaceLcdDisplay::SetChatMessage(const char* role, const char* content) {
    (void)role; (void)content;  // face has no room for subtitles
}

void FaceLcdDisplay::SetMoving(bool moving) {
    moving_.store(moving);
    ESP_LOGI(TAG, "moving: %s", moving ? "on" : "off");
}

void FaceLcdDisplay::SetPlayingMusic(bool playing) {
    playing_music_.store(playing);
    ESP_LOGI(TAG, "playing_music: %s", playing ? "on" : "off");
}

void FaceLcdDisplay::SetDanceMood(DanceMood mood) {
    dance_mood_.store(mood);
    ESP_LOGI(TAG, "dance_mood: %d", static_cast<int>(mood));
}

void FaceLcdDisplay::SetFollowMood(FollowMood mood) {
    follow_mood_.store(mood);
    ESP_LOGI(TAG, "follow_mood: %d", static_cast<int>(mood));
}

// ---------------------------------------------------------------------------
// Effective-state resolution: music > error > moving > voice state.
// Kept as a helper so AnimTick() can call it every frame without caring
// which inputs actually changed.
// ---------------------------------------------------------------------------
void FaceLcdDisplay::RecomputeEffectiveState() {
    FaceState desired;
    // Dance mood wins over everything else. When the dance ends, the
    // DanceManager sets kNone and we fall back to the normal priority.
    DanceMood  dm = dance_mood_.load();
    FollowMood fm = follow_mood_.load();
    if (dm != DanceMood::kNone) {
        switch (dm) {
            case DanceMood::kReady:       desired = FaceState::kDanceReady; break;
            case DanceMood::kExcited:     desired = FaceState::kDanceExcited; break;
            case DanceMood::kSpin:        desired = FaceState::kDanceSpin; break;
            case DanceMood::kHighlight:   desired = FaceState::kDanceHighlight; break;
            case DanceMood::kEndPose:     desired = FaceState::kDanceEndPose; break;
            case DanceMood::kSad:         desired = FaceState::kDanceSad; break;
            case DanceMood::kEmbarrassed: desired = FaceState::kDanceEmbarrassed; break;
            default:                      desired = voice_state_.load(); break;
        }
    }
    else if (fm != FollowMood::kNone) {
        switch (fm) {
            case FollowMood::kWaitHand: desired = FaceState::kFollowWait; break;
            case FollowMood::kActive:   desired = FaceState::kFollowActive; break;
            case FollowMood::kTooClose: desired = FaceState::kFollowTooClose; break;
            case FollowMood::kLost:     desired = FaceState::kFollowLost; break;
            default:                    desired = voice_state_.load(); break;
        }
    }
    else if (playing_music_.load()) desired = FaceState::kMusic;
    else if (errored_.load())       desired = FaceState::kError;
    else if (moving_.load())        desired = FaceState::kMoving;
    else                            desired = voice_state_.load();

    if (desired == current_state_) return;

    transition_from_  = current_state_;
    transition_to_    = desired;
    transition_start_ = lv_tick_get();
    current_state_    = desired;
    state_enter_tick_ = transition_start_;
}

// ---------------------------------------------------------------------------
// Declarative per-state pose. Everything that isn't overridden here (bounce,
// vibration, blink, dot march) is layered on afterwards in AnimTick().
// ---------------------------------------------------------------------------
FaceLcdDisplay::FaceParams FaceLcdDisplay::ParamsForState(FaceState s) {
    // Reasonable defaults tuned for 160x80.
    FaceParams p = {
        /*eye_w*/           24,
        /*eye_h*/           24,
        /*eye_spacing*/     52,
        /*eye_cy*/          34,
        /*smile_eye_opa*/   0.0f,
        /*mouth_w*/         0,
        /*mouth_h*/         0,
        /*mouth_cy*/        62,
        /*mouth_smile_opa*/ 0.0f,
    };
    switch (s) {
        case FaceState::kIdle:
            // Round eyes, no mouth -- calm resting pose.
            break;
        case FaceState::kListening:
            // Same as idle; pulse + slower blink live in AnimTick overlays.
            break;
        case FaceState::kSpeaking:
            // Happy arc eyes (opacity up). Mouth starts at the "small open"
            // shape (matches the steady-state of the AnimTick overlay), so
            // the lerp INTO speaking grows the mouth from 0 -> small, and
            // the lerp OUT of speaking shrinks back to 0 cleanly. The
            // discrete shape stepping happens in the AnimTick overlay.
            p.smile_eye_opa = 1.0f;
            p.mouth_w = 14; p.mouth_h = 4;
            break;
        case FaceState::kThinking:
            // Circles; side-shift + dots handled as overlays.
            p.mouth_w = 12; p.mouth_h = 2;
            break;
        case FaceState::kMoving:
            // Narrowed focused eyes. Per spec, mouth is hidden in MOVING:
            // the user's design rules say the small-line mouth should only
            // appear during SPEAKING.
            p.eye_w = 28; p.eye_h = 12; p.eye_spacing = 54;
            p.mouth_w = 0; p.mouth_h = 0;
            break;
        case FaceState::kMusic:
            // Closed happy eyes (arc visible, bodies shrunk to nothing) +
            // a smiling mouth arc. Bounce overlay adds the rhythm.
            p.eye_w = 0; p.eye_h = 0;
            p.smile_eye_opa = 1.0f;
            p.mouth_smile_opa = 1.0f;
            p.mouth_w = 32; p.mouth_h = 0;
            break;
        case FaceState::kError:
            // Asymmetric tiny dots. Fast blink handled as overlay.
            p.eye_w = 8; p.eye_h = 8; p.eye_spacing = 46;
            p.mouth_w = 18; p.mouth_h = 2;
            break;

        // --- Dance poses ----------------------------------------------------
        case FaceState::kDanceReady:
            // Slightly wider eyes + tiny smile — "watch this!"
            p.eye_w = 22; p.eye_h = 22;
            p.smile_eye_opa = 0.0f;
            p.mouth_w = 18; p.mouth_h = 2;
            break;
        case FaceState::kDanceExcited:
            // Big round eyes, no mouth — overlays bounce the whole face.
            p.eye_w = 30; p.eye_h = 30; p.eye_spacing = 54;
            p.smile_eye_opa = 0.0f;
            p.mouth_w = 0; p.mouth_h = 0;
            break;
        case FaceState::kDanceSpin:
            // Narrowed horizontal "whoosh" eyes; overlay shifts them L/R.
            p.eye_w = 30; p.eye_h = 10; p.eye_spacing = 50;
            p.mouth_w = 18; p.mouth_h = 2;
            break;
        case FaceState::kDanceHighlight:
            // Huge eyes + smile arcs — the climax frame.
            p.eye_w = 32; p.eye_h = 32; p.eye_spacing = 56;
            p.smile_eye_opa = 0.0f;
            p.mouth_smile_opa = 1.0f;
            p.mouth_w = 36; p.mouth_h = 0;
            break;
        case FaceState::kDanceEndPose:
            // Closed happy arc eyes + full smile — "ta-da!"
            p.eye_w = 0; p.eye_h = 0;
            p.smile_eye_opa = 1.0f;
            p.mouth_smile_opa = 1.0f;
            p.mouth_w = 40; p.mouth_h = 0;
            break;
        case FaceState::kDanceSad:
            // Thin squinted eyes + tiny frown (flat mouth line, low).
            p.eye_w = 18; p.eye_h = 6; p.eye_spacing = 48; p.eye_cy = 38;
            p.mouth_w = 16; p.mouth_h = 2; p.mouth_cy = 66;
            break;
        case FaceState::kDanceEmbarrassed:
            // Small eyes that look down + short mouth — a sheepish look.
            p.eye_w = 14; p.eye_h = 14; p.eye_spacing = 50; p.eye_cy = 40;
            p.mouth_w = 14; p.mouth_h = 2;
            break;

        // --- Follow moods ---------------------------------------------------
        // Per spec: mouth is hidden in FOLLOW modes (waiting / active / lost).
        // Only the surprised "too close" pose keeps a mouth, and that's an
        // intentional "o!" expression, not a syllabling speaking mouth.
        case FaceState::kFollowWait:
            // "Looking forward, ready" — tall oval eyes, no mouth.
            p.eye_w = 20; p.eye_h = 22; p.eye_spacing = 52;
            p.mouth_w = 0; p.mouth_h = 0;
            break;
        case FaceState::kFollowActive:
            // Horizontal focused eyes, no mouth. Overlay adds a bounce.
            p.eye_w = 30; p.eye_h = 12; p.eye_spacing = 54;
            p.mouth_w = 0; p.mouth_h = 0;
            break;
        case FaceState::kFollowTooClose:
            // Big surprised round eyes + open mouth ("o!").
            p.eye_w = 30; p.eye_h = 30; p.eye_spacing = 58;
            p.mouth_w = 10; p.mouth_h = 10; p.mouth_cy = 62;
            break;
        case FaceState::kFollowLost:
            // Confused — asymmetric eye heights, no mouth.
            p.eye_w = 22; p.eye_h = 14; p.eye_spacing = 50;
            p.mouth_w = 0; p.mouth_h = 0;
            break;
    }
    return p;
}

FaceLcdDisplay::FaceParams FaceLcdDisplay::Interp(const FaceParams& a,
                                                  const FaceParams& b,
                                                  float t) {
    FaceParams r;
    r.eye_w           = Lerp(a.eye_w,           b.eye_w,           t);
    r.eye_h           = Lerp(a.eye_h,           b.eye_h,           t);
    r.eye_spacing     = Lerp(a.eye_spacing,     b.eye_spacing,     t);
    r.eye_cy          = Lerp(a.eye_cy,          b.eye_cy,          t);
    r.smile_eye_opa   = Lerp(a.smile_eye_opa,   b.smile_eye_opa,   t);
    r.mouth_w         = Lerp(a.mouth_w,         b.mouth_w,         t);
    r.mouth_h         = Lerp(a.mouth_h,         b.mouth_h,         t);
    r.mouth_cy        = Lerp(a.mouth_cy,        b.mouth_cy,        t);
    r.mouth_smile_opa = Lerp(a.mouth_smile_opa, b.mouth_smile_opa, t);
    return r;
}

// ---------------------------------------------------------------------------
// Push a fully-resolved pose (base + overlays) onto the LVGL widgets. Called
// from the 30 FPS timer only.
// ---------------------------------------------------------------------------
void FaceLcdDisplay::ApplyPose(const FaceParams& p,
                               int extra_offset_x, int extra_offset_y,
                               int left_extra_dy, int right_extra_dy,
                               int dots_visible) {
    const int W = width();
    const int H = height();
    const int cx = W / 2;

    // Clamp dimensions to sane minimums so LVGL doesn't silently drop zero-
    // sized children.
    auto clamp_min = [](int v, int lo) { return v < lo ? lo : v; };

    const int eye_w = clamp_min(p.eye_w, 0);
    const int eye_h = clamp_min(p.eye_h, 0);

    for (int i = 0; i < 2; ++i) {
        const int sign = (i == 0) ? -1 : +1;
        const int ex = cx + sign * p.eye_spacing / 2 + extra_offset_x - eye_w / 2;
        const int ey = p.eye_cy + extra_offset_y - eye_h / 2
                     + (i == 0 ? left_extra_dy : right_extra_dy);

        if (eye_w > 0 && eye_h > 0) {
            lv_obj_clear_flag(eye_body_[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_size(eye_body_[i], eye_w, eye_h);
            lv_obj_set_pos(eye_body_[i], ex, ey);
        } else {
            lv_obj_add_flag(eye_body_[i], LV_OBJ_FLAG_HIDDEN);
        }

        // Smile-eye arc sits over the same center as the body.
        const int arc_w = 30, arc_h = 18;
        const int ax = cx + sign * p.eye_spacing / 2 + extra_offset_x - arc_w / 2;
        const int ay = p.eye_cy + extra_offset_y - arc_h / 2
                     + (i == 0 ? left_extra_dy : right_extra_dy);
        lv_obj_set_pos(eye_arc_[i], ax, ay);
        lv_obj_set_style_opa(eye_arc_[i],
            static_cast<lv_opa_t>(std::clamp(p.smile_eye_opa, 0.0f, 1.0f) * 255),
            0);
    }

    // Mouth rect.
    if (p.mouth_w > 0 && p.mouth_h > 0) {
        lv_obj_clear_flag(mouth_rect_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_size(mouth_rect_, p.mouth_w, p.mouth_h);
        lv_obj_set_pos(mouth_rect_,
                       cx - p.mouth_w / 2 + extra_offset_x,
                       p.mouth_cy - p.mouth_h / 2 + extra_offset_y);
    } else {
        lv_obj_add_flag(mouth_rect_, LV_OBJ_FLAG_HIDDEN);
    }

    // Mouth smile arc (centered on the mouth anchor line).
    const int marc_w = 40, marc_h = 20;
    lv_obj_set_pos(mouth_arc_,
                   cx - marc_w / 2 + extra_offset_x,
                   p.mouth_cy - marc_h / 2 + extra_offset_y);
    lv_obj_set_style_opa(mouth_arc_,
        static_cast<lv_opa_t>(std::clamp(p.mouth_smile_opa, 0.0f, 1.0f) * 255),
        0);

    // Thinking dots. dots_visible == -1 -> all hidden. Otherwise show the
    // first (dots_visible) dots, 0..3.
    for (int i = 0; i < 3; ++i) {
        if (dots_visible > i) {
            lv_obj_clear_flag(dot_[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_pos(dot_[i], cx - 10 + i * 8, H - 14);
        } else {
            lv_obj_add_flag(dot_[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

// ---------------------------------------------------------------------------
// Render tick. Flow:
//   1) Recompute effective state (picks up any atomic flag flips).
//   2) Interpolate the "from" pose into the "to" pose over kTransitionMs.
//   3) Layer state-specific overlays (blink, pulse, mouth flap, bounce,
//      vibration, dot march, asymmetry).
//   4) Push everything onto the widgets in one atomic pass.
// ---------------------------------------------------------------------------
void FaceLcdDisplay::AnimTickThunk(lv_timer_t* t) {
    static_cast<FaceLcdDisplay*>(lv_timer_get_user_data(t))->AnimTick();
}

void FaceLcdDisplay::AnimTick() {
    if (face_root_ == nullptr) return;

    RecomputeEffectiveState();

    const uint32_t now = lv_tick_get();

    // 1) transition blend
    float t_raw = (now - transition_start_) / static_cast<float>(kTransitionMs);
    float t     = Smoothstep(t_raw);
    FaceParams base = Interp(ParamsForState(transition_from_),
                             ParamsForState(transition_to_),
                             t);

    // 2) overlays (per active state only; use `current_state_` so that
    // mid-transition you already get a taste of the new behavior, which
    // feels more alive than strictly waiting for the lerp to finish).
    int ox = 0, oy = 0;
    int dl = 0, dr = 0;
    int dots = -1;
    const uint32_t since_enter = now - state_enter_tick_;

    switch (current_state_) {
        case FaceState::kIdle:
        case FaceState::kListening: {
            // Slow blink. Interval depends on state.
            const int min_ms = (current_state_ == FaceState::kIdle)
                               ? kIdleBlinkMinMs : kListenBlinkMinMs;
            const int max_ms = (current_state_ == FaceState::kIdle)
                               ? kIdleBlinkMaxMs : kListenBlinkMaxMs;
            if (!in_blink_ && now >= next_blink_tick_) {
                in_blink_       = true;
                blink_end_tick_ = now + kBlinkCloseMs;
            }
            if (in_blink_) {
                // Squish eyes to a thin line.
                base.eye_h = 3;
                if (now >= blink_end_tick_) {
                    in_blink_        = false;
                    next_blink_tick_ = now + min_ms + (esp_random() % (max_ms - min_ms));
                }
            }
            // Pulse (listening only).
            if (current_state_ == FaceState::kListening) {
                float phase = (since_enter % 2000) / 2000.0f * 2.0f * static_cast<float>(M_PI);
                int pulse = static_cast<int>(std::sin(phase) * 1.5f);
                base.eye_w += pulse;
                base.eye_h += pulse;
            }
            break;
        }
        case FaceState::kSpeaking: {
            // Discrete-shape mouth stepper. We pick one of three target
            // shapes every 90..130 ms (random jitter inside that band, so
            // it doesn't read as mechanical):
            //
            //   0: closed       — thin horizontal line
            //   1: small open   — short oval
            //   2: wide open    — taller rounded oval
            //
            // The cycle pattern [small, wide, small, closed] gives a
            // syllable-like rhythm: open up, peak, settle back, brief
            // closure between syllables. Repeat. Compared with the old
            // continuous sine, it reads much more like "talking" because
            // each shape transition is a clear visual beat.
            //
            // Audio level isn't surfaced to the firmware in this build, so
            // we use the pure animation path; if AudioService ever exposes
            // a peak-volume signal we'd just bias `target_h` by it here.
            if (now >= mouth_step_until_) {
                mouth_step_idx_   = (mouth_step_idx_ + 1) & 0x03;  // 0..3
                mouth_step_until_ = now + 90 + (esp_random() % 41);  // 90..130 ms
            }
            int target_w = 14, target_h = 4;
            switch (mouth_step_idx_ & 0x03) {
                case 0: target_w = 14; target_h = 4;  break;  // small open
                case 1: target_w = 16; target_h = 9;  break;  // wide open
                case 2: target_w = 14; target_h = 4;  break;  // small open
                case 3: target_w = 18; target_h = 2;  break;  // closed line
            }
            // Smooth a tiny bit toward target so we don't snap hard between
            // shapes — single-frame easing (factor 0.5) is plenty at 30 FPS.
            base.mouth_w = (base.mouth_w + target_w) / 2;
            base.mouth_h = (base.mouth_h + target_h) / 2;

            // Eyes pulse very gently with the voice, ~1.25 Hz, ±1 px.
            // Subtle on a 80 px display; reads as "warm/friendly engaged"
            // without overpowering the mouth animation.
            float eye_phase = (since_enter % 800) / 800.0f
                              * 2.0f * static_cast<float>(M_PI);
            int pulse = static_cast<int>(std::sin(eye_phase) * 1.0f);
            base.eye_w += pulse;
            base.eye_h += pulse;
            break;
        }
        case FaceState::kThinking: {
            // Side-shift eyes at 0.2 Hz + ellipsis dots sequencing at 2 Hz.
            float phase = (since_enter % 5000) / 5000.0f * 2.0f * static_cast<float>(M_PI);
            ox = static_cast<int>(std::sin(phase) * 5.0f);
            dots = static_cast<int>(since_enter / 500) % 4;  // 0..3
            break;
        }
        case FaceState::kMoving: {
            // Vibration: random 1-2 px jitter. Not deterministic, but at
            // 30 FPS it reads as "engaged".
            ox = (esp_random() % 3) - 1;
            oy = (esp_random() % 3) - 1;
            break;
        }
        case FaceState::kMusic: {
            // Rhythmic bounce at ~1.7 Hz. |sin| so it's all-up, like the
            // body is following a beat.
            float phase = (since_enter % 600) / 600.0f * static_cast<float>(M_PI);
            oy = -static_cast<int>(std::sin(phase) * 5.0f);
            break;
        }
        case FaceState::kError: {
            // Fast blink + tilt one eye up and the other down.
            const uint32_t ph = since_enter % kErrorBlinkPeriodMs;
            if (ph < kErrorBlinkPeriodMs / 2) {
                base.eye_h = 2;
            }
            dl = -2; dr = +2;
            break;
        }

        // --- Dance overlays -------------------------------------------------
        case FaceState::kDanceReady: {
            // Gentle head-tilt: alternating +/- left/right eye dy.
            float phase = (since_enter % 800) / 800.0f * 2.0f * (float)M_PI;
            dl = static_cast<int>(std::sin(phase) * 3.0f);
            dr = -dl;
            break;
        }
        case FaceState::kDanceExcited: {
            // Springy bounce + slight wobble. Feels like the face "hops"
            // with the motor rhythm.
            float phase = (since_enter % 300) / 300.0f * (float)M_PI;
            oy = -static_cast<int>(std::sin(phase) * 4.0f);
            ox = (static_cast<int>(since_enter / 150) % 2) ? 1 : -1;
            break;
        }
        case FaceState::kDanceSpin: {
            // Eyes sweep side to side quickly, like they're tracking the spin.
            float phase = (since_enter % 500) / 500.0f * 2.0f * (float)M_PI;
            ox = static_cast<int>(std::sin(phase) * 8.0f);
            break;
        }
        case FaceState::kDanceHighlight: {
            // Fast blink at ~6 Hz for the climax; tiny shake for impact.
            const uint32_t ph = since_enter % 160;
            if (ph < 60) base.eye_h = 4;
            ox = (esp_random() % 3) - 1;
            oy = (esp_random() % 3) - 1;
            break;
        }
        case FaceState::kDanceEndPose: {
            // Small settle bounce that fades out — "holds the pose".
            if (since_enter < 500) {
                float phase = since_enter / 500.0f * (float)M_PI;
                oy = -static_cast<int>(std::sin(phase) * 3.0f);
            }
            break;
        }
        case FaceState::kDanceSad: {
            // Slow droop blink; eyes slump down over 1s.
            if (since_enter < 1000) {
                oy = static_cast<int>((since_enter / 1000.0f) * 3.0f);
            } else {
                oy = 3;
            }
            break;
        }
        case FaceState::kDanceEmbarrassed: {
            // Tiny shy shake side to side.
            float phase = (since_enter % 600) / 600.0f * 2.0f * (float)M_PI;
            ox = static_cast<int>(std::sin(phase) * 2.0f);
            break;
        }

        // --- Follow overlays ------------------------------------------------
        case FaceState::kFollowWait: {
            // Slow blink, like kIdle but less frequent, to "wait calmly".
            if (!in_blink_ && now >= next_blink_tick_) {
                in_blink_       = true;
                blink_end_tick_ = now + kBlinkCloseMs;
            }
            if (in_blink_) {
                base.eye_h = 3;
                if (now >= blink_end_tick_) {
                    in_blink_ = false;
                    // 2.5 s..4.0 s between blinks.
                    next_blink_tick_ = now + 2500 + (esp_random() % 1500);
                }
            }
            break;
        }
        case FaceState::kFollowActive: {
            // Tiny "engaged" bounce at ~2 Hz. Keeps the face alive while
            // the motors are cruising forward at a constant speed.
            float phase = (since_enter % 500) / 500.0f * (float)M_PI;
            oy = -static_cast<int>(std::sin(phase) * 2.0f);
            break;
        }
        case FaceState::kFollowTooClose: {
            // Slight recoil wobble (you're too close to me!).
            ox = (esp_random() % 3) - 1;
            break;
        }
        case FaceState::kFollowLost: {
            // Slow side-to-side "where'd you go?" scan.
            float phase = (since_enter % 2000) / 2000.0f * 2.0f * (float)M_PI;
            ox = static_cast<int>(std::sin(phase) * 6.0f);
            break;
        }
    }

    ApplyPose(base, ox, oy, dl, dr, dots);
}

// ---------------------------------------------------------------------------
void FaceLcdDisplay::RegisterFaceMcpTools() {
    if (mcp_tools_registered_) return;
    mcp_tools_registered_ = true;

    auto& mcp = McpServer::GetInstance();
    PropertyList props({ Property("playing", kPropertyTypeBoolean) });
    mcp.AddTool("self.face.set_playing_music",
        "Toggle the on-screen face to a 'playing music' expression. "
        "Call with playing=true right before starting music playback, "
        "and playing=false when the song stops or is interrupted.",
        props,
        [this](const PropertyList& p) -> ReturnValue {
            bool playing = p["playing"].value<bool>();
            SetPlayingMusic(playing);
            return std::string(playing ? "music_face_on" : "music_face_off");
        });

    // self.face.set_dance_mood — used by the server-side dance handler to
    // play a reaction face after the user rates the dance (end_pose for a
    // high score, embarrassed for mid, sad for low, none to clear). The
    // dance routine itself drives the face directly from the FreeRTOS
    // task, so callers do NOT need to call this during the dance.
    PropertyList dance_props({ Property("mood", kPropertyTypeString) });
    mcp.AddTool("self.face.set_dance_mood",
        "Set the robot face to a short dance-reaction mood. "
        "mood: none|ready|excited|spin|highlight|end_pose|sad|embarrassed. "
        "Use 'end_pose' for a proud happy reaction, 'embarrassed' for a "
        "sheepish reaction, 'sad' for a sad reaction, and 'none' to clear.",
        dance_props,
        [this](const PropertyList& p) -> ReturnValue {
            std::string m = p["mood"].value<std::string>();
            DanceMood mood = DanceMood::kNone;
            if      (m == "ready")       mood = DanceMood::kReady;
            else if (m == "excited")     mood = DanceMood::kExcited;
            else if (m == "spin")        mood = DanceMood::kSpin;
            else if (m == "highlight")   mood = DanceMood::kHighlight;
            else if (m == "end_pose")    mood = DanceMood::kEndPose;
            else if (m == "sad")         mood = DanceMood::kSad;
            else if (m == "embarrassed") mood = DanceMood::kEmbarrassed;
            else if (m == "none")        mood = DanceMood::kNone;
            else return std::string("unknown mood");
            SetDanceMood(mood);
            return std::string("ok");
        });
}
