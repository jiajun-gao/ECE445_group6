#include "target_detector.h"
#include <string.h>

void detector_face_init(void);
void detector_face_run(camera_fb_t *fb, target_t *out);
void detector_color_run(camera_fb_t *fb, target_t *out);

static detector_kind_t s_kind = DET_KIND_COLOR;

void detector_init(detector_kind_t k) {
    s_kind = k;
    if (k == DET_KIND_FACE) detector_face_init();
}

void detector_run(camera_fb_t *fb, target_t *out) {
    if (s_kind == DET_KIND_FACE) detector_face_run(fb, out);
    else                         detector_color_run(fb, out);
}
