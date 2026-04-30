// Face detector wrapper. Uses Espressif esp-dl `human_face_detect` model.
// Add component dependency: idf.py add-dependency "espressif/esp-dl"
// and pull a face_detect example for exact model API (varies by version).
//
// For portability, this file ships a stub that returns "no detection" unless
// the ESP_DL_FACE symbol is defined and you wire up the real calls.
#include "target_detector.h"
#include "config.h"
#include <string.h>

#ifdef ESP_DL_FACE
// Example sketch — adjust to the esp-dl API version you pull in.
// #include "human_face_detect_msr01.hpp"
// static HumanFaceDetectMSR01 *g_det;

void detector_face_init(void) {
    // g_det = new HumanFaceDetectMSR01(0.3F, 0.3F, 10, 0.3F);
}

void detector_face_run(camera_fb_t *fb, target_t *out) {
    memset(out, 0, sizeof(*out));
    // auto boxes = g_det->infer((uint16_t*)fb->buf, {IMG_H, IMG_W, 3});
    // if (boxes.size() == 0) return;
    // pick largest:
    // auto &b = *std::max_element(boxes.begin(), boxes.end(),
    //     [](auto &a, auto &b){ return (a.box[2]-a.box[0])*(a.box[3]-a.box[1])
    //                             < (b.box[2]-b.box[0])*(b.box[3]-b.box[1]); });
    // out->valid = true;
    // out->cx = 0.5f * (b.box[0] + b.box[2]);
    // out->cy = 0.5f * (b.box[1] + b.box[3]);
    // out->w  = b.box[2] - b.box[0];
    // out->h  = b.box[3] - b.box[1];
    // out->confidence = b.score;
}
#else
void detector_face_init(void) {}
void detector_face_run(camera_fb_t *fb, target_t *out) {
    (void)fb;
    memset(out, 0, sizeof(*out));   // stub: no face until esp-dl wired
}
#endif
