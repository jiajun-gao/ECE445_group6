// Largest-color-blob detector on RGB565 QVGA. Works great for a marker like
// an orange wristband. Tune HSV_* to your marker under your room lighting.
#include "target_detector.h"
#include "config.h"
#include <string.h>

// HSV target band (user-tunable). H ~= orange.
static const int H_MIN = 5,   H_MAX = 25;
static const int S_MIN = 120, V_MIN = 80;

static inline void rgb565_to_rgb888(uint16_t px, uint8_t *r, uint8_t *g, uint8_t *b) {
    // NB: esp_camera packs RGB565 big-endian per byte pair.
    uint16_t p = (px >> 8) | (px << 8);
    *r = (p >> 11) & 0x1F; *r = (*r * 255 + 15) / 31;
    *g = (p >> 5)  & 0x3F; *g = (*g * 255 + 31) / 63;
    *b =  p        & 0x1F; *b = (*b * 255 + 15) / 31;
}

static inline void rgb_to_hsv(uint8_t r, uint8_t g, uint8_t b, int *H, int *S, int *V) {
    uint8_t mx = r > g ? (r > b ? r : b) : (g > b ? g : b);
    uint8_t mn = r < g ? (r < b ? r : b) : (g < b ? g : b);
    *V = mx;
    int d = mx - mn;
    *S = mx ? (d * 255 / mx) : 0;
    int h = 0;
    if (d == 0) h = 0;
    else if (mx == r) h = (43 * (g - b) / d + 256) % 256;
    else if (mx == g) h = 85 + 43 * (b - r) / d;
    else              h = 171 + 43 * (r - g) / d;
    *H = h * 180 / 255;
}

void detector_color_run(camera_fb_t *fb, target_t *out) {
    memset(out, 0, sizeof(*out));
    if (!fb || fb->len < IMG_W * IMG_H * 2) return;
    const uint16_t *px = (const uint16_t *)fb->buf;

    // moments for centroid + bbox
    uint32_t cnt = 0, sx = 0, sy = 0;
    int minx = IMG_W, maxx = 0, miny = IMG_H, maxy = 0;

    // subsample by 2 for speed
    for (int y = 0; y < IMG_H; y += 2) {
        for (int x = 0; x < IMG_W; x += 2) {
            uint8_t r, g, b;
            rgb565_to_rgb888(px[y * IMG_W + x], &r, &g, &b);
            int H, S, V;
            rgb_to_hsv(r, g, b, &H, &S, &V);
            if (H >= H_MIN && H <= H_MAX && S >= S_MIN && V >= V_MIN) {
                cnt++; sx += x; sy += y;
                if (x < minx) minx = x; if (x > maxx) maxx = x;
                if (y < miny) miny = y; if (y > maxy) maxy = y;
            }
        }
    }
    if (cnt < 120) return; // too few pixels -> no target
    out->valid = true;
    out->cx = (float)sx / cnt;
    out->cy = (float)sy / cnt;
    out->w  = (float)(maxx - minx);
    out->h  = (float)(maxy - miny);
    out->confidence = cnt > 2000 ? 1.0f : cnt / 2000.0f;
}
