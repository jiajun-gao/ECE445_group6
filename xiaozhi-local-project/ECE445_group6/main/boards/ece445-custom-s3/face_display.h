#ifndef ECE445_FACE_DISPLAY_H_
#define ECE445_FACE_DISPLAY_H_

#include <atomic>
#include <lvgl.h>
#include <string>

#include "display/lcd_display.h"

// FaceLcdDisplay: full-screen programmatic "robot face" renderer for the
// 160x80 ST7735 panel. No emoji PNGs, no fonts -- the face is drawn from
// LVGL primitives (rounded rectangles + arcs) and animated by a 33 ms
// lv_timer, so it stays fluid while the audio / robot stack changes state
// underneath.
//
// Public API mirrors the pre-emoji version; callers (Application, robot_app,
// MCP tools) don't need to know the rendering changed.
class FaceLcdDisplay : public SpiLcdDisplay {
public:
    FaceLcdDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel,
                   int width, int height, int offset_x, int offset_y,
                   bool mirror_x, bool mirror_y, bool swap_xy);
    ~FaceLcdDisplay() override;

    void SetupUI() override;
    void SetStatus(const char* status) override;
    void SetEmotion(const char* emotion) override;
    void SetChatMessage(const char* role, const char* content) override;

    void SetMoving(bool moving) override;
    void SetPlayingMusic(bool playing) override;

    // Dance moods — short-lived face overrides used by DanceManager while a
    // dance routine is playing. Explicit "none" clears the override and lets
    // the regular voice/music/moving/error state take the face back. Values
    // are kept in sync with robot::DanceMood; see dance_manager.h.
    enum class DanceMood : uint8_t {
        kNone = 0,
        kReady,
        kExcited,
        kSpin,
        kHighlight,
        kEndPose,
        kSad,
        kEmbarrassed,
    };
    // Safe to call from any task (FreeRTOS dance task, server message
    // handler, LVGL timer, ...). Only flips an atomic; AnimTick picks it up.
    void SetDanceMood(DanceMood mood);

    // Follow moods — short-lived face overrides published by FollowManager.
    // Separate channel from DanceMood because priority differs: dance beats
    // follow (a user who says "dance" while following should see the dance
    // face). Values mirror robot::FollowMood; see follow_manager.h.
    enum class FollowMood : uint8_t {
        kNone = 0,
        kWaitHand,   // eyes forward, slow blink
        kActive,     // focused, tiny bounce
        kTooClose,   // surprised (analog variant only today)
        kLost,       // confused / asymmetric
    };
    void SetFollowMood(FollowMood mood);

    // Registers MCP tools so the LLM can drive the face (music toggle today,
    // trivially extensible to more expressions later). Idempotent.
    void RegisterFaceMcpTools();

private:
    // High-level face states. Priority (top wins):
    //   dance mood (if != kNone)
    //   > follow mood (if != kNone)
    //   > kError > kMusic > kMoving
    //   > voice state (kSpeaking/kListening/kThinking/kIdle)
    // Dance and follow moods get their own FaceState entries so AnimTick
    // can give each one its own pose + overlay without a separate branch.
    enum class FaceState {
        kIdle,
        kListening,
        kSpeaking,
        kThinking,
        kMoving,
        kMusic,
        kError,
        kDanceReady,
        kDanceExcited,
        kDanceSpin,
        kDanceHighlight,
        kDanceEndPose,
        kDanceSad,
        kDanceEmbarrassed,
        kFollowWait,
        kFollowActive,
        kFollowTooClose,
        kFollowLost,
    };

    // Pose (geometric parameters) that each state declaratively describes.
    // AnimTick() linearly interpolates between two of these over 300 ms on
    // state transitions, then layers per-state overlays on top.
    struct FaceParams {
        int  eye_w;          // rect size; radius is LV_RADIUS_CIRCLE so it always looks like a pill/circle
        int  eye_h;
        int  eye_spacing;    // horizontal distance between the two eye centers
        int  eye_cy;         // vertical center of both eyes
        float smile_eye_opa; // 0..1 opacity of the arched "happy eye" overlay
        int  mouth_w;
        int  mouth_h;        // 0 = mouth hidden
        int  mouth_cy;
        float mouth_smile_opa; // 0..1 opacity of the smile-arc mouth overlay
    };

    // --- build/teardown ----------------------------------------------------
    void BuildFaceWidgets();

    // --- state intake (may be called from any task) ------------------------
    void RecomputeEffectiveState();

    // --- render tick (runs on LVGL task, owns the port lock) ---------------
    static void AnimTickThunk(lv_timer_t* t);
    void AnimTick();

    static FaceParams ParamsForState(FaceState s);
    static FaceParams Interp(const FaceParams& a, const FaceParams& b, float t);
    void ApplyPose(const FaceParams& p,
                   int extra_offset_x, int extra_offset_y,
                   int left_extra_dy, int right_extra_dy,
                   int dots_visible);

    // --- widgets -----------------------------------------------------------
    lv_obj_t* face_root_    = nullptr;
    lv_obj_t* eye_body_[2]  = {nullptr, nullptr};  // rounded rect (circle/oval/line/dot)
    lv_obj_t* eye_arc_[2]   = {nullptr, nullptr};  // smile-eye overlay (closed happy arc)
    lv_obj_t* mouth_rect_   = nullptr;             // line / open mouth
    lv_obj_t* mouth_arc_    = nullptr;             // smile arc
    lv_obj_t* dot_[3]       = {nullptr, nullptr, nullptr};
    lv_timer_t* anim_timer_ = nullptr;

    // --- state machine -----------------------------------------------------
    std::atomic<FaceState> voice_state_  {FaceState::kIdle};
    std::atomic<bool>      moving_       {false};
    std::atomic<bool>      playing_music_{false};
    std::atomic<bool>      errored_      {false};
    // Dance mood override. When != kNone it wins over every other source.
    std::atomic<DanceMood>  dance_mood_  {DanceMood::kNone};
    // Follow mood override. Lower priority than dance, higher than the
    // normal voice/music/moving/error resolution.
    std::atomic<FollowMood> follow_mood_ {FollowMood::kNone};

    FaceState  current_state_     = FaceState::kIdle;
    FaceState  transition_from_   = FaceState::kIdle;
    FaceState  transition_to_     = FaceState::kIdle;
    uint32_t   transition_start_  = 0;     // lv_tick_get() ms
    uint32_t   state_enter_tick_  = 0;
    uint32_t   next_blink_tick_   = 0;
    bool       in_blink_          = false;
    uint32_t   blink_end_tick_    = 0;
    // Speaking-mouth stepper. The kSpeaking overlay cycles through a small
    // vocabulary of mouth shapes (closed line / small oval / wide oval) and
    // re-picks one every ~80–150 ms so the face looks like it's syllabling.
    // We track our own phase here instead of using a continuous sine so
    // each step lands on a discrete shape — much more "lip-sync" feel than
    // the previous 4 Hz analog flap.
    uint8_t    mouth_step_idx_    = 0;     // index into the cycle
    uint32_t   mouth_step_until_  = 0;     // lv_tick_get() ms when next step fires
    bool       mcp_tools_registered_ = false;
};

#endif  // ECE445_FACE_DISPLAY_H_
