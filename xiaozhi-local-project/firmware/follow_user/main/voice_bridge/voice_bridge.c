#include "voice_bridge.h"
#include "mode_manager.h"
#include "motor_control.h"

void voice_on_follow_me(void)      { mode_set(MODE_FOLLOW_USER); }
void voice_on_stop_following(void) { mode_set(MODE_IDLE); }
void voice_on_manual_mode(void)    { mode_set(MODE_MANUAL); }
void voice_on_ai_mode(void)        { mode_set(MODE_AI); }
void voice_on_stop(void) {
    motor_cmd_t c = { .verb = CMD_STOP, .pwm = 0, .source = CMD_SRC_MANUAL };
    motor_control_submit(&c);
}
