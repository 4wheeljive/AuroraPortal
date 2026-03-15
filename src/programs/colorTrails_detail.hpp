#pragma once

// ****************** DANGER: UNDER CONSTRUCTION  ******************

//=====================================================================================
//  colortrails began with a FastLED Reddit post by u/StefanPetrick: 
//  https://www.reddit.com/r/FastLED/comments/1rny5j3/i_used_codex_for_the_first_time/
//   
//  I had Claude help me (1) port it to a FastLED/Arduino-friendly/C++ sketch and then 
//  (2) implement that as this new "colorTrails" AuroraPortal prorgam. 
//  As Stefan has shared subsequent ideas, I've been implementing them here.
//  
//=====================================================================================

#include "bleControl.h"

namespace colorTrails {

    constexpr float CT_PI = 3.14159265358979f;

    bool colorTrailsInstance = false;
    uint16_t (*xyFunc)(uint8_t x, uint8_t y);

    // Floating-point RGB grid, row-major [y][x].
    // Two copies: g* is the live buffer, t* is scratch for advection.
    static float gR[HEIGHT][WIDTH], gG[HEIGHT][WIDTH], gB[HEIGHT][WIDTH];
    static float tR[HEIGHT][WIDTH], tG[HEIGHT][WIDTH], tB[HEIGHT][WIDTH];

    static unsigned long t0;
    static unsigned long lastFrameMs;

    uint8_t lastMode;
    
    void setModeDefaults();

    void initColorTrails(uint16_t (*xy_func)(uint8_t, uint8_t)) {
        colorTrailsInstance = true;
        xyFunc = xy_func;
        for (int y = 0; y < HEIGHT; y++)
            for (int x = 0; x < WIDTH; x++)
                gR[y][x] = gG[y][x] = gB[y][x] = 0.0f;
        t0 = fl::millis();
        lastFrameMs = t0;
        lastMode = -1;
        setModeDefaults();
    }

    
    // EMITTERS ====================================================================

    /*  Quoting/paraphrasing Stefan:
        The emitter (aka injector / color source) is anything that is drawn directly.
        Think of it as a pencil or painbrush or paint spray gun. But it can be anything, for example:
        - bouncing balls
        - an audio-reactive pulsating ring
        - Animartrix output that still contains some black areas 
        The emitter geometry can be static or dynamic (e.g., fixed rectangular border vs. orbiting dots)
        The emitter may use one or more noise functions in its internal pipeline 
        The emitter output could be displayed and would be a normal animation
    */

    class Emitter {

        // emitter engine (or an object that holds or points to that)
        // mechanism to hold or indicate indicate which UI paramters are used by the emitter
        // hooks to one or more modulators?
        // hooks to one or more noise functions?  

    };

    Emitter OrbitalDots;
    Emitter LissajousLine;
    Emitter BorderRect;

    
    // COLOR FLOW FIELDS ==========================================================
    
    /*  Quoting/paraphrasing Stefan:

    You can think of a ColorFlowField as an invisible wind that moves the previous pixels and blends them together.
    Each ColorFlowField follows its own different rules and can produce characteristic outputs:
    - spirals
    - vortices / flows towards or away from an origin (which could be staionary or dynamic)
    - polar warp flows
    - directional / geometric flows?
    - smoke/vapor?

    A ColorFlowField may use one or more noise functions in its internal pipeline 

    The current/initial NoiseFlowField is especially interesting because it creates these emergent, dynamic,
    turbulence-like shapes that remind one of a fluid simulation. 
    It is the result of wind blowing from two directions with varying intensities.

    */


    // MODULATORS ==========================================================

    class Modulator {


    }; 



    // COLOR FLOW FIELDS ==========================================================

    class ColorFlowField {

        // add an object to hold the flow field's advection engine
        // mechanism to hold or indicate which UI paramters are used by this flow field  

    };

    ColorFlowField NoiseFlow;
    // future plans
    //ColorFlowField SpiralFlow;
    //ColorFlowField CenterFlow;
    //ColorFlowField OutwardFlow;
    //ColorFlowField PolarWarpFlow;
    //ColorFlowField DirectionalFlow;


    // VIZUALIZER CONFIG ===========================================================

    /* this becomes the new basic unit of organization for colorTrails
        It holds:
        - (current applicable universal objects (e.g., fadeRate?) 
        - Emitter
        - ColorFlowField



*/

   
    // NOISE FUNCTIONS ===========================================================

    // 1D Perlin noise ---------------------------------------
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

    // 2D Perlin noise ---------------------------------------
    class Perlin2D {
    public:
        void init(uint32_t seed) {
            uint8_t p[256];
            for (int i = 0; i < 256; i++) p[i] = (uint8_t)i;
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

        float noise(float x, float y) const {
            int xi = ((int)fl::floorf(x)) & 255;
            int yi = ((int)fl::floorf(y)) & 255;
            float xf = x - fl::floorf(x);
            float yf = y - fl::floorf(y);
            float u = xf * xf * xf * (xf * (xf * 6.0f - 15.0f) + 10.0f);
            float v = yf * yf * yf * (yf * (yf * 6.0f - 15.0f) + 10.0f);

            int aa = perm[perm[xi]     + yi];
            int ab = perm[perm[xi]     + yi + 1];
            int ba = perm[perm[xi + 1] + yi];
            int bb = perm[perm[xi + 1] + yi + 1];

            float x1 = lerp(grad(aa, xf, yf),         grad(ba, xf - 1.0f, yf),        u);
            float x2 = lerp(grad(ab, xf, yf - 1.0f),  grad(bb, xf - 1.0f, yf - 1.0f), u);
            return lerp(x1, x2, v);
        }

    private:
        uint8_t perm[512];

        static float grad(int h, float x, float y) {
            switch (h & 7) {
                case 0: return  x + y;
                case 1: return -x + y;
                case 2: return  x - y;
                case 3: return -x - y;
                case 4: return  x;
                case 5: return -x;
                case 6: return  y;
                default: return -y;
            }
        }

        static float lerp(float a, float b, float t) {
            return a + t * (b - a);
        }
    };

    // -------------------------------------------------------------------
   
    static Perlin1D noiseX, noiseY;
    static Perlin1D ampVarX, ampVarY;
    static Perlin2D noise2X, noise2Y;

    static float xProf[WIDTH];    // one noise value per column
    static float yProf[HEIGHT];   // one noise value per row
    

    // HELPER FUNCTIONS ==============================================================

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

    // MODE PARAMETERS =========================================================

    // mode = injector / emitter / color source
    


    // ColorFlowField = advection parameters



    struct ModeParams {
        
        float fadeRate = -999.f;  	// Per-frame fade factor
        float xSpeed = -999.f;   	// Noise scroll speed  (column axis)
        float ySpeed = -999.f;   	// Noise scroll speed  (row axis)
        float xAmplitude = -999.f;   // Noise amplitude     (column axis)
        float yAmplitude = -999.f;   // Noise amplitude     (row axis)
        float xFrequency = -999.f;   // Noise spatial scale (column axis) (aka "xScale")
        float yFrequency = -999.f;   // Noise spatial scale (row axis) (aka "yScale")
        float xShift = -999.f;   // Max horizontal shift per row  (pixels)
        float yShift = -999.f;   // Max vertical shift per column (pixels)
		
		// Unique to mode 0 (orbital)
		float orbitSpeed = -999.f;   // Circle orbit angular speed
        float colorSpeed = -999.f;   // Rainbow hue rotation speed
        float circleDiam = -999.f;   // Injected circle diameter
        float orbitDiam = -999.f;    // Orbit diameter

        // Unique to mode 1 (Lissajous line)
        float endpointSpeed = -999.f; 	// Lissajous endpoint speed
        float colorShift = -999.f; 		// Rainbow color shift along line

        // Smear mode: 0 = normal advection, 1 = reversed X noise profile
        uint8_t smearMode = 0;

        // Noise mode: 0 = 1D Perlin
        //             1 = 2D Perlin
        //uint8_t noiseMode = 0;

        // Amplitude modulation: slow 1D Perlin noise modulates xAmplitude/yAmplitude
        float variationIntensity = -999.f;  	// Depth of amplitude modulation (0 = off)
        float variationSpeed = -999.f;  		// Temporal speed of the variation noise
        uint8_t modulateAmplitude = 99;  // Modulation method (0 = off)
    };

    ModeParams modeDefaults[3];  // per-mode defaults
    ModeParams params;          // active working copy

    void setModeDefaults() {

       // Mode 0 (orbital)
       // Included in BLE/UI system
            modeDefaults[0].fadeRate = 99.922f;
            modeDefaults[0].xShift = 1.8f;
            modeDefaults[0].yShift = 1.8f;
            modeDefaults[0].xSpeed = -1.73f;
            modeDefaults[0].ySpeed = -1.72f;
            modeDefaults[0].xAmplitude = 1.00f;
            modeDefaults[0].yAmplitude = 1.00f;
            modeDefaults[0].xFrequency = .33f;
            modeDefaults[0].yFrequency = .32f;
            modeDefaults[0].orbitSpeed = 0.35f;
            modeDefaults[0].colorSpeed = 0.10f; 
            modeDefaults[0].orbitDiam = 10.f;
            modeDefaults[0].circleDiam = 1.5f;
            //modeDefaults[0].modulateAmplitude = 0; 
            //modeDefaults[0].variationIntensity = 4.00f; 
            //modeDefaults[0].variationSpeed = 1.00f;
            // Not included in BLE/UI system
            //modeDefaults[0].noiseMode = 0; // 1DPerlin
            modeDefaults[0].smearMode = 0; // normal advection 
        
       // Mode 1 (lissajous): 
       // Included in BLE/UI system
            modeDefaults[1].fadeRate = 99.922f;
            modeDefaults[1].xShift = 1.8f;
            modeDefaults[1].yShift = 1.8f;
            modeDefaults[1].xSpeed = 0.10f;
            modeDefaults[1].ySpeed = 0.10f;
            modeDefaults[1].xAmplitude = 1.00f;
            modeDefaults[1].yAmplitude = 1.00f;
            modeDefaults[1].xFrequency = .23f;
            modeDefaults[1].yFrequency = .23f;
            modeDefaults[1].endpointSpeed = 0.35f;
            modeDefaults[1].colorShift = 0.10f;
            //modeDefaults[1].modulateAmplitude = 0;
            //modeDefaults[2].variationIntensity = 4.00f; 
            //modeDefaults[2].variationSpeed = 1.00f;
         // Not included in BLE/UI system   
            //modeDefaults[1].noiseMode = 0; // 1DPerlin
            modeDefaults[1].smearMode = 1; // reversed x profile
        
       // Mode 2 (bordersong):
       // Included in BLE/UI system
            modeDefaults[2].fadeRate = 99.922f;
            modeDefaults[2].xShift = 1.8f;
            modeDefaults[2].yShift = 1.8f;
            modeDefaults[2].xSpeed = 0.10f;
            modeDefaults[2].ySpeed = 0.10f;
            modeDefaults[2].xAmplitude = 0.75f;
            modeDefaults[2].yAmplitude = 0.75f;
            modeDefaults[2].xFrequency = .33f;
            modeDefaults[2].yFrequency = .32f;
            modeDefaults[2].colorShift = 0.10f;
            modeDefaults[2].modulateAmplitude = 0;
            modeDefaults[2].variationIntensity = 4.00f; 
            modeDefaults[2].variationSpeed = 1.00f;
       // Not included in BLE/UI system
            //modeDefaults[2].noiseMode = 0; // 0 = 1DPerlin
            modeDefaults[2].smearMode = 0; // 0 = normal advection 

    }

    void applyModeDefaults(const ModeParams& md) {

        // Directly set mode parameters that do not have UI control
        //params.noiseMode = md.noiseMode;
        params.smearMode = md.smearMode;

        // Indirectly set mode parameters that do have UI control by setting applicable cVar values
        if (md.fadeRate > -999.f) cFadeRate = md.fadeRate;
        if (md.xSpeed > -999.f) cXSpeed = md.xSpeed;   
        if (md.ySpeed > -999.f) cYSpeed = md.ySpeed;   
        if (md.xAmplitude > -999.f) cXAmplitude = md.xAmplitude;
        if (md.yAmplitude > -999.f) cYAmplitude = md.yAmplitude;
        if (md.xFrequency > -999.f) cXFrequency = md.xFrequency;
        if (md.yFrequency > -999.f) cYFrequency = md.yFrequency;
        if (md.xShift > -999.f) cXShift = md.xShift;
        if (md.yShift > -999.f) cYShift = md.yShift;
        if (md.orbitSpeed > -999.f) cOrbitSpeed = md.orbitSpeed;
        if (md.colorSpeed > -999.f) cColorSpeed = md.colorSpeed;
        if (md.circleDiam > -999.f) cCircleDiam = md.circleDiam;
        if (md.orbitDiam > -999.f) cOrbitDiam = md.orbitDiam;
        if (md.endpointSpeed > -999.f) cEndpointSpeed = md.endpointSpeed;
        if (md.colorShift > -999.f)  cColorShift = md.colorShift;      // ⚠️ 0 is valid
        if (md.variationIntensity > -999.f) cVariationIntensity = md.variationIntensity;
        if (md.variationSpeed > -999.f)  cVariationSpeed = md.variationSpeed;
        if (md.modulateAmplitude < 99) cModulateAmplitude = md.modulateAmplitude;  

        // Push values to UI
        sendVisualizerState();

    }; 

    void updateModeParams() {
        params.fadeRate = cFadeRate;
        params.xSpeed = cXSpeed;
        params.ySpeed = cYSpeed;
        params.xAmplitude = cXAmplitude;
        params.yAmplitude = cYAmplitude;
        params.xFrequency = cXFrequency;
        params.yFrequency = cYFrequency;
        params.xShift = cXShift;
        params.yShift = cYShift;
        params.orbitSpeed = cOrbitSpeed;
        params.colorSpeed = cColorSpeed;
        params.circleDiam = cCircleDiam;
        params.orbitDiam = cOrbitDiam;
        params.endpointSpeed = cEndpointSpeed;
        params.colorShift = cColorShift;
        params.variationIntensity = cVariationIntensity;
        params.variationSpeed = cVariationSpeed;
        params.modulateAmplitude = cModulateAmplitude;

    }

    
    // VISUALIZER FUNCTIONS =====================================================================

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

    // Build one noise profile using 2D Perlin: spatial on x-axis, temporal scroll on y-axis.
    static void sampleProfile2D(const Perlin2D &n, float t, float speed,
                                float amp, float scale, int count, float *out) {
        const float freq   = 0.23f;
        const float scrollY = t * speed;
        for (int i = 0; i < count; i++) {
            float v = n.noise(i * freq * scale, scrollY);
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

    
    // ADVECTION WILL BECOME PART OF THE COLOR FLOW FIELD STRUCTURE, AND THE FUNCTION BELOW IS THE ADVECTION METHOD FOR THE NOISE FLOW FIELD  
    
    // Advection -------------------------------------------------------------------

    // Two-pass fractional advection (bilinear interpolation) + per-pixel fade.
    //   Pass 1: shift each row horizontally using the Y-noise profile.
    //   Pass 2: shift each column vertically using the X-noise profile, then dim.
    static void advectAndDim(float dt) {
        // The original Python applied fadeRate once per frame at 60 FPS.
        // Scale the exponent by actual dt so decay rate is frame-rate-independent.
        float fadePerSec = fl::powf(params.fadeRate * 0.01f, 60.0f); // decay over 1 second at 60fps
        float fade = fl::powf(fadePerSec, dt);                       // scale to actual elapsed time

        // Pass 1 — horizontal row shift  (Y-noise drives X movement)
        for (int y = 0; y < HEIGHT; y++) {
            float sh = yProf[y] * params.xShift;
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
            float sh = xProf[x] * params.yShift;
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

    // COLOR INJECTORS/EMITTERS =====================================================

    // Inject orbiting circles
    static void injectOrbitingCircles(float t) { 
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
    }

    // Inject a Lissajous line: two endpoints trace independent sine paths.
    static void injectLissajousLine(float t) {
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

    // Inject rainbow colors along the grid border (perimeter walk).
    static void injectRainbowBorder(float t) {
        const int total = 2 * (WIDTH + HEIGHT) - 4;
        int idx = 0;
        // Top edge: left to right
        for (int x = 0; x < WIDTH; x++) {
            CRGB c = rainbow(t, params.colorShift, (float)idx / total);
            gR[0][x] = c.r; gG[0][x] = c.g; gB[0][x] = c.b;
            idx++;
        }
        // Right edge: top+1 to bottom
        for (int y = 1; y < HEIGHT; y++) {
            CRGB c = rainbow(t, params.colorShift, (float)idx / total);
            gR[y][WIDTH-1] = c.r; gG[y][WIDTH-1] = c.g; gB[y][WIDTH-1] = c.b;
            idx++;
        }
        // Bottom edge: right-1 to left
        for (int x = WIDTH - 2; x >= 0; x--) {
            CRGB c = rainbow(t, params.colorShift, (float)idx / total);
            gR[HEIGHT-1][x] = c.r; gG[HEIGHT-1][x] = c.g; gB[HEIGHT-1][x] = c.b;
            idx++;
        }
        // Left edge: bottom-1 to top+1
        for (int y = HEIGHT - 2; y > 0; y--) {
            CRGB c = rainbow(t, params.colorShift, (float)idx / total);
            gR[y][0] = c.r; gG[y][0] = c.g; gB[y][0] = c.b;
            idx++;
        }
    }


    // MAIN PROGRAM LOOP ===============================================================

    void runColorTrails() {
        
        unsigned long now = fl::millis();
        float dt = (now - lastFrameMs) * 0.001f;
        lastFrameMs = now;
        float t = (now - t0) * 0.001f;
        
        if (MODE != lastMode) {
            applyModeDefaults(modeDefaults[MODE]);
            lastMode = MODE;
        }

        updateModeParams();

        /* where do these go?
            noiseX.init(42);
            noiseY.init(1337);
            noise2X.init(42);
            noise2Y.init(1337);
            ampVarX.init(101);
            ampVarY.init(202);
        */

        // Amplitude modulation ----------------------------
        if (params.modulateAmplitude) {
            float nVarX = ampVarX.noise(t * 0.16f * params.variationSpeed);
            float nVarY = ampVarY.noise(t * 0.13f * params.variationSpeed + 17.0f);

            float selfMod = 0.5f + 0.5f * ((nVarX + nVarY) * 0.5f);
            float effVariation = params.variationIntensity * selfMod;

            params.xAmplitude = clampf(params.xAmplitude + nVarX * 0.45f * effVariation, 0.10f, 1.0f);
            params.yAmplitude = clampf(params.yAmplitude + nVarY * 0.45f * effVariation, 0.10f, 1.0f);
        }

        // Noise profiles -----------------------------------
        // 2D Perlin  
        sampleProfile2D(noise2X, t, params.xSpeed, params.xAmplitude,
                        params.xFrequency, WIDTH,  xProf);
        sampleProfile2D(noise2Y, t, params.ySpeed, params.yAmplitude,
                        params.yFrequency, HEIGHT, yProf);
    
        // 1D Perlin
        sampleProfile(noiseX, t, params.xSpeed, params.xAmplitude,
                    params.xFrequency, WIDTH,  xProf);
        sampleProfile(noiseY, t, params.ySpeed, params.yAmplitude,
                    params.yFrequency, HEIGHT, yProf);
    
        // Apply smear to noise profiles ------------------
        if (params.smearMode == 1) {
            // Reverse X profile (originally from lissajous sketch)
            for (int i = 0; i < WIDTH / 2; i++) {
                float tmp = xProf[i];
                xProf[i] = xProf[WIDTH - 1 - i];
                xProf[WIDTH - 1 - i] = tmp;
            }
        }

        // Inject color source/emitter --------------------
        switch (MODE) {
        default:
        case 0: {
            // 3 orbiting circles (orbital)
            injectOrbitingCircles(t);
            break;
        }
        case 1:
            // Lissajous line
            injectLissajousLine(t);
            break;

        case 2:
            // Rainbow border rectangle
            injectRainbowBorder(t);
            break;
        }

        // Advect + fade --------------------------------------
        advectAndDim(dt);

        // Copy float grid to LED array -----------------------
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
