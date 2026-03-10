#pragma once
/*
// colorTrails_detail.hpp  —  Color Trails LED visualization
//   This visualizer began with a post by u/StefanPetrick on the FastLED Reddit: 
//   https://www.reddit.com/r/FastLED/comments/1rny5j3/i_used_codex_for_the_first_time/
//   
//   I had Claude help me (1) port it to a FastLED/Arduino-friendly/C++ sketch and then 
//   (2) implement that as this new "colorTrails" AuroraPortal prorgam
//
//   Preliminary video captures:
//      https://youtu.be/qczTTGWb2Yo
//      https://youtu.be/R-iENxidWkE
*/

#include "bleControl.h"

// ============================================================
//  Tunable parameters  (defaults taken from the Python sketch)
// ============================================================

struct CTParams {
    float xSpeed     = -1.73f;   // Noise scroll speed  (column axis)
    float xAmplitude =  0.80f;   // Noise amplitude     (column axis)
    float ySpeed     = -1.72f;   // Noise scroll speed  (row axis)
    float yAmplitude =  0.80f;   // Noise amplitude     (row axis)
    float fadePct    = 99.922f;  // Per-frame fade %    (99 – 100)
    float xScale     =  0.33f;   // Noise spatial scale (column axis)
    float yScale     =  0.32f;   // Noise spatial scale (row axis)
    float rowShiftPx =  1.8f;    // Max horizontal shift per row  (pixels)
    float colShiftPx =  1.8f;    // Max vertical shift per column (pixels)

    // Mode 0: orbiting circles
    float orbitSpeed =  1.76f;   // Circle orbit angular speed
    float colorSpeed =  0.76f;   // Rainbow hue rotation speed
    float circleDiam =  1.5f;    // Injected circle diameter
    float orbitDiam  = 10.0f;    // Orbit diameter

    // Mode 1: Lissajous line
    float endpointSpeed = 0.35f; // Lissajous endpoint speed
    float colorShift    = 0.10f; // Rainbow color shift along line
};

// ============================================================
//  1-D Perlin noise  (mirrors the Python Perlin1D class)
// ============================================================

class Perlin1D {
public:
    void init(uint32_t seed) {
        uint8_t p[256];
        for (int i = 0; i < 256; i++) p[i] = (uint8_t)i;
        // Fisher-Yates shuffle with a simple LCG
        uint32_t s = seed;
        for (int i = 255; i > 0; i--) {
            s = s * 1664525u + 1013904223u;
            int j = (int)((s >> 16) % (uint32_t)(i + 1));
            uint8_t tmp = p[i]; p[i] = p[j]; p[j] = tmp;
        }
        for (int i = 0; i < 256; i++) {
            perm[i]       = p[i];
            perm[i + 256] = p[i];
        }
    }

    // Classic 1-D Perlin: fade, grad, lerp — returns roughly [-1, 1].
    float noise(float x) const {
        int   xi = ((int)fl::floorf(x)) & 255;
        float xf = x - fl::floorf(x);
        float u  = xf * xf * xf * (xf * (xf * 6.0f - 15.0f) + 10.0f);
        float ga = (perm[xi]     & 1) ? -xf         :  xf;
        float gb = (perm[xi + 1] & 1) ? -(xf - 1.f) : (xf - 1.f);
        return ga + u * (gb - ga);
    }

private:
    uint8_t perm[512];
};

// ============================================================
//  Color Trails effect
// ============================================================

namespace colorTrails {

constexpr float CT_PI = 3.14159265358979f;

bool colorTrailsInstance = false;
uint16_t (*xyFunc)(uint8_t x, uint8_t y);

// ---- internal state ----

static Perlin1D noiseX, noiseY;
static CTParams params;
static unsigned long t0;
static unsigned long lastFrameMs;

// Floating-point RGB grid, row-major [y][x].
// Two copies: g* is the live buffer, t* is scratch for advection.
static float gR[HEIGHT][WIDTH], gG[HEIGHT][WIDTH], gB[HEIGHT][WIDTH];
static float tR[HEIGHT][WIDTH], tG[HEIGHT][WIDTH], tB[HEIGHT][WIDTH];

static float xProf[WIDTH];    // one noise value per column
static float yProf[HEIGHT];   // one noise value per row

// ---- helpers ----

// Non-negative float modulo (matches Python's % for positive m).
static inline float fmodPos(float x, float m) {
    float r = fl::fmodf(x, m);
    return r < 0.0f ? r + m : r;
}

static inline float clampf(float v, float lo, float hi) {
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

static inline uint8_t f2u8(float v) {
    int i = (int)v;
    if (i < 0)   return 0;
    if (i > 255) return 255;
    return (uint8_t)i;
}

// Build one noise profile (one value per row **or** per column).
static void sampleProfile(const Perlin1D &n, float t, float speed,
                           float amp, float scale, int count, float *out) {
    const float freq  = 0.23f;
    const float phase = t * speed;
    for (int i = 0; i < count; i++) {
        float v = n.noise(i * freq * scale + phase);
        out[i]  = clampf(v * amp, -1.0f, 1.0f);
    }
}

// Draw an anti-aliased sub-pixel circle into the float grid.
static void drawCircle(float cx, float cy, float diam,
                        uint8_t cr, uint8_t cg, uint8_t cb) {
    float rad = diam * 0.5f;
    int x0 = max(0,               (int)fl::floorf(cx - rad - 1.0f));
    int x1 = min((int)WIDTH  - 1, (int)fl::ceilf (cx + rad + 1.0f));
    int y0 = max(0,               (int)fl::floorf(cy - rad - 1.0f));
    int y1 = min((int)HEIGHT - 1, (int)fl::ceilf (cy + rad + 1.0f));

    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            float dx   = (x + 0.5f) - cx;
            float dy   = (y + 0.5f) - cy;
            float dist = fl::sqrtf(dx * dx + dy * dy);
            float cov  = clampf(rad + 0.5f - dist, 0.0f, 1.0f);
            if (cov <= 0.0f) continue;
            float inv = 1.0f - cov;
            gR[y][x] = gR[y][x] * inv + cr * cov;
            gG[y][x] = gG[y][x] * inv + cg * cov;
            gB[y][x] = gB[y][x] * inv + cb * cov;
        }
    }
}

// Blend a single pixel with weighted alpha (used by line and disc drawing).
static void blendPixelWeighted(int px, int py,
                                uint8_t cr, uint8_t cg, uint8_t cb,
                                float w) {
    if (px < 0 || px >= WIDTH || py < 0 || py >= HEIGHT) return;
    w = clampf(w, 0.0f, 1.0f);
    if (w <= 0.0f) return;
    float inv = 1.0f - w;
    gR[py][px] = gR[py][px] * inv + cr * w;
    gG[py][px] = gG[py][px] * inv + cg * w;
    gB[py][px] = gB[py][px] * inv + cb * w;
}

// Anti-aliased disc at a sub-pixel position (for line endpoints).
static void drawAAEndpointDisc(float cx, float cy,
                                uint8_t cr, uint8_t cg, uint8_t cb,
                                float radius = 0.85f) {
    int x0 = max(0,               (int)fl::floorf(cx - radius - 1.0f));
    int x1 = min((int)WIDTH  - 1, (int)fl::ceilf (cx + radius + 1.0f));
    int y0 = max(0,               (int)fl::floorf(cy - radius - 1.0f));
    int y1 = min((int)HEIGHT - 1, (int)fl::ceilf (cy + radius + 1.0f));
    for (int py = y0; py <= y1; py++) {
        for (int px = x0; px <= x1; px++) {
            float dx   = (px + 0.5f) - cx;
            float dy   = (py + 0.5f) - cy;
            float dist = fl::sqrtf(dx * dx + dy * dy);
            float w    = clampf(radius + 0.5f - dist, 0.0f, 1.0f);
            blendPixelWeighted(px, py, cr, cg, cb, w);
        }
    }
}

// Full-saturation, full-brightness rainbow from a continuous hue.
static CRGB rainbow(float t, float speed, float phase) {
    float hue = fmodPos(t * speed + phase, 1.0f);
    CHSV hsv((uint8_t)(hue * 255.0f), 255, 255);
    CRGB rgb;
    hsv2rgb_rainbow(hsv, rgb);
    return rgb;
}

// Anti-aliased sub-pixel line with rainbow color varying along its length.
static void drawAASubpixelLine(float x0, float y0, float x1, float y1,
                                float t, float colorShift) {
    float dx = x1 - x0;
    float dy = y1 - y0;
    float maxd = fl::fabsf(dx) > fl::fabsf(dy) ? fl::fabsf(dx) : fl::fabsf(dy);
    int steps = max(1, (int)(maxd * 3.0f));
    for (int i = 0; i <= steps; i++) {
        float u  = (float)i / (float)steps;
        float x  = x0 + dx * u;
        float y  = y0 + dy * u;
        int   xi = (int)fl::floorf(x);
        int   yi = (int)fl::floorf(y);
        float fx = x - xi;
        float fy = y - yi;
        CRGB c = rainbow(t, params.colorShift, u);
        blendPixelWeighted(xi,     yi,     c.r, c.g, c.b, (1.0f - fx) * (1.0f - fy));
        blendPixelWeighted(xi + 1, yi,     c.r, c.g, c.b, fx * (1.0f - fy));
        blendPixelWeighted(xi,     yi + 1, c.r, c.g, c.b, (1.0f - fx) * fy);
        blendPixelWeighted(xi + 1, yi + 1, c.r, c.g, c.b, fx * fy);
    }
}

// Inject a Lissajous line: two endpoints trace independent sine paths.
static void injectLissajousLine(float t, float colorShift, float endpointSpeed) {
    const float c = (MIN_DIMENSION - 1) * 0.5f;
    float s = params.endpointSpeed;
    const float amp = (MIN_DIMENSION - 4) * 0.5f;

    float lx1 = c + (amp + 1.5f) * fl::sinf(t * s * 1.13f + 0.20f); // was 11.5
    float ly1 = c + (amp + 0.5f) * fl::sinf(t * s * 1.71f + 1.30f); // was 10.5
    float lx2 = c + (amp + 2.0f) * fl::sinf(t * s * 1.89f + 2.20f); // was 12.0
    float ly2 = c + (amp + 1.0f) * fl::sinf(t * s * 1.37f + 0.70f); // was 11.0

    drawAASubpixelLine(lx1, ly1, lx2, ly2, t, params.colorShift);
    CRGB ca = rainbow(t, params.colorShift, 0.0f);
    CRGB cb = rainbow(t, params.colorShift, 1.0f);
    drawAAEndpointDisc(lx1, ly1, ca.r, ca.g, ca.b, 0.85f);
    drawAAEndpointDisc(lx2, ly2, cb.r, cb.g, cb.b, 0.85f);
}

// Two-pass fractional advection (bilinear interpolation) + per-pixel fade.
//   Pass 1: shift each row horizontally using the Y-noise profile.
//   Pass 2: shift each column vertically using the X-noise profile, then dim.
static void advectAndDim(float dt) {
    // The original Python applied fadePct once per frame at 60 FPS.
    // Scale the exponent by actual dt so decay rate is frame-rate-independent.
    float fadePerSec = fl::powf(params.fadePct * 0.01f, 60.0f); // decay over 1 second at 60fps
    float fade = fl::powf(fadePerSec, dt);                       // scale to actual elapsed time

    // Pass 1 — horizontal row shift  (Y-noise drives X movement)
    for (int y = 0; y < HEIGHT; y++) {
        float sh = yProf[y] * params.rowShiftPx;
        for (int x = 0; x < WIDTH; x++) {
            float sx  = fmodPos((float)x - sh, (float)WIDTH);
            int   ix0 = (int)fl::floorf(sx) % WIDTH;
            int   ix1 = (ix0 + 1) % WIDTH;
            float f   = sx - fl::floorf(sx);
            float inv = 1.0f - f;
            tR[y][x] = gR[y][ix0] * inv + gR[y][ix1] * f;
            tG[y][x] = gG[y][ix0] * inv + gG[y][ix1] * f;
            tB[y][x] = gB[y][ix0] * inv + gB[y][ix1] * f;
        }
    }

    // Pass 2 — vertical column shift  (X-noise drives Y movement) + dim
    for (int x = 0; x < WIDTH; x++) {
        float sh = xProf[x] * params.colShiftPx;
        for (int y = 0; y < HEIGHT; y++) {
            float sy  = fmodPos((float)y - sh, (float)HEIGHT);
            int   iy0 = (int)fl::floorf(sy) % HEIGHT;
            int   iy1 = (iy0 + 1) % HEIGHT;
            float f   = sy - fl::floorf(sy);
            float inv = 1.0f - f;
            // truncate to integer — Python's Pygame surface stores uint8,
            // so int(value) kills sub-1.0 residuals every frame.
            gR[y][x] = fl::floorf((tR[iy0][x] * inv + tR[iy1][x] * f) * fade);
            gG[y][x] = fl::floorf((tG[iy0][x] * inv + tG[iy1][x] * f) * fade);
            gB[y][x] = fl::floorf((tB[iy0][x] * inv + tB[iy1][x] * f) * fade);
        }
    }
}


// ============================================================
//  Public API
// ============================================================

void initColorTrails(uint16_t (*xy_func)(uint8_t, uint8_t)) {
    colorTrailsInstance = true;
    xyFunc = xy_func;

    noiseX.init(42);
    noiseY.init(1337);
    for (int y = 0; y < HEIGHT; y++)
        for (int x = 0; x < WIDTH; x++)
            gR[y][x] = gG[y][x] = gB[y][x] = 0.0f;
    t0 = fl::millis();
    lastFrameMs = t0;
}

void runColorTrails() {
    // ---- apply BLE parameters ----
    params.fadePct = cFadePct;
    params.xScale = cXScale;
    params.yScale = cYScale;
    params.orbitSpeed = cOrbitSpeed;
    params.orbitDiam = cOrbitDiam;
    params.colorSpeed = cColorSpeed;
    params.circleDiam = cCircleDiam;
    params.rowShiftPx = cRowShiftPx;
    params.colShiftPx = cColShiftPx;
    params.endpointSpeed = cEndpointSpeed;
    params.colorShift = cColorShift;
    
    unsigned long now = fl::millis();
    float dt = (now - lastFrameMs) * 0.001f;
    lastFrameMs = now;
    float t = (now - t0) * 0.001f;

    // ---- noise profiles for this frame ----
    sampleProfile(noiseX, t, params.xSpeed, params.xAmplitude,
                  params.xScale, WIDTH,  xProf);
    sampleProfile(noiseY, t, params.ySpeed, params.yAmplitude,
                  params.yScale, HEIGHT, yProf);

    // Mode 1: reverse the X profile (lissajous sketch behaviour)
    if (MODE == 1) {
        for (int i = 0; i < WIDTH / 2; i++) {
            float tmp = xProf[i];
            xProf[i] = xProf[WIDTH - 1 - i];
            xProf[WIDTH - 1 - i] = tmp;
        }
    }

    // ---- inject color source ----
    switch (MODE) {
    default:
    case 0: {
        // 3 orbiting circles (orbital)
        float ocx  = WIDTH  * 0.5f - 0.5f;
        float ocy  = HEIGHT * 0.5f - 0.5f;
        float orad = params.orbitDiam * 0.5f;
        float base = t * params.orbitSpeed;
        for (int i = 0; i < 3; i++) {
            float a  = base + i * (2.0f * CT_PI / 3.0f);
            float cx = ocx + fl::cosf(a) * orad;
            float cy = ocy + fl::sinf(a) * orad;
            CRGB c = rainbow(t, params.colorSpeed, i / 3.0f);
            drawCircle(cx, cy, params.circleDiam, c.r, c.g, c.b);
        }
        break;
    }
    case 1:
        // Lissajous line
        injectLissajousLine(t, params.colorShift, params.endpointSpeed);
        break;
    }

    // ---- advect + fade ----
    advectAndDim(dt);

    // ---- copy float grid to LED array ----
    for (uint8_t y = 0; y < HEIGHT; y++) {
        for (uint8_t x = 0; x < WIDTH; x++) {
            uint16_t idx = xyFunc(x, y);
            leds[idx].r = f2u8(gR[y][x]);
            leds[idx].g = f2u8(gG[y][x]);
            leds[idx].b = f2u8(gB[y][x]);
        }
    }
}

} // namespace colorTrails
