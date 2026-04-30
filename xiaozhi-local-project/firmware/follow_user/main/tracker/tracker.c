#include "tracker.h"
#include "config.h"
#include <string.h>

void tracker_init(tracker_t *t) {
    memset(t, 0, sizeof(*t));
}

void tracker_update(tracker_t *t, const target_t *det) {
    if (det && det->valid) {
        // simple EMA on cx/w to smooth jitter
        if (t->last.valid) {
            const float a = 0.5f;
            t->last.cx = a * det->cx + (1 - a) * t->last.cx;
            t->last.w  = a * det->w  + (1 - a) * t->last.w;
            t->last.cy = det->cy;
            t->last.h  = det->h;
            t->last.confidence = det->confidence;
            t->last.ts_ms = det->ts_ms;
        } else {
            t->last = *det;
        }
        t->last.valid = true;
        t->lost_frames = 0;
    } else {
        if (t->last.valid) t->lost_frames++;
    }
}

bool tracker_get_error(const tracker_t *t, float *err_norm, float *size_ratio) {
    if (!t->last.valid || t->lost_frames > 0) {
        // caller decides what to do when lost; we still expose last known
        if (!t->last.valid) return false;
    }
    float err = (t->last.cx - (IMG_W * 0.5f)) / (IMG_W * 0.5f);
    if (err >  1.0f) err =  1.0f;
    if (err < -1.0f) err = -1.0f;
    *err_norm   = err;
    *size_ratio = t->last.w / (float)IMG_W;
    return true;
}
