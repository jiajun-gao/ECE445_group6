#pragma once
// Hooks that your existing voice stack (xiaozhi) should call when it
// recognizes these intents. Keep this file small — it's a shim.
void voice_on_follow_me(void);
void voice_on_stop_following(void);
void voice_on_manual_mode(void);
void voice_on_ai_mode(void);
void voice_on_stop(void);
