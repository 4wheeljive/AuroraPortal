#pragma once
/*
My personal learning playground for working with:
    FastLED Adapter for the animartrix fx library.
    Copyright Stefan Petrick 2023.
    Adapted to C++ by Netmindz 2023.
    Adapted to FastLED by Zach Vorhies 2024.
    For details on the animartrix library and licensing information, 
    see https://github.com/FastLED/FastLED/blob/master/src/fx/2d/animartrix_detail.hpp
*/

/*
  ___        _            ___  ______ _____    _
 / _ \      (_)          / _ \ | ___ \_   _|  (_)
/ /_\ \_ __  _ _ __ ___ / /_\ \| |_/ / | |_ __ ___  __
|  _  | '_ \| | '_ ` _ \|  _  ||    /  | | '__| \ \/ /
| | | | | | | | | | | | | | | || |\ \  | | |  | |>  <
\_| |_/_| |_|_|_| |_| |_\_| |_/\_| \_| \_/_|  |_/_/\_\

by Stefan Petrick 2023.

High quality LED animations for your next project.

This is a Shader and 5D Coordinate Mapper made for realtime
rendering of generative animations & artistic dynamic visuals.

This is also a modular animation synthesizer with waveform
generators, oscillators, filters, modulators, noise generators,
compressors... and much more.   

VO.42 beta version

This code is licenced under a Creative Commons Attribution
License CC BY-NC 3.0
*/

#include "fl/stl/vector.h"
#include "fl/stl/math.h"
#include "fl/stl/stdint.h"
#include "fl/sin32.h"
#include "audio/audioProcessing.h"
#include "audio/avHelpers.h"

#ifndef ANIMARTRIX_INTERNAL
#error                                                                         \
    "This file is not meant to be included directly. Include animartrix.hpp instead."
#endif

// Copyright Stefen Petrick 2023.
// Adapted to C++ by Netmindz 2023.
// Adapted to FastLED by Zach Vorhies 2024.
// Licensed under the Creative Commons Attribution License CC BY-NC 3.0
// https://creativecommons.org/licenses/by-nc/3.0/
// This header is distributed with FastLED but has a different license that
// limits commercial use. If you include this high quality LED animation library
// in your project, you must agree to the licensing terms. It is not included by
// FastLED by default, you must include it manually. Setting
// FASTLED_ANIMARTRIX_LICENSING_AGREEMENT=1 will indicate that you agree to the
// licensing terms of the ANIMartRIX library for non commercial use only.
//
// Like the rest of FastLED, this header is free for non-commercial use and
// licensed under the Creative Commons Attribution License CC BY-NC 3.0. If you
// are just making art, then by all means do what you want with this library and
// you can stop reading now. If you are using this header for commercial
// purposes, then you need to contact Stefan Petrick for a commercial use
// license.

//#define FASTLED_ANIMARTRIX_LICENSING_AGREEMENT 1
// Setting this to 1 means you agree to the licensing terms of the ANIMartRIX
// library for non commercial use only.
#if defined(FASTLED_ANIMARTRIX_LICENSING_AGREEMENT) ||                         \
    (FASTLED_ANIMARTRIX_LICENSING_AGREEMENT != 0)
#warning                                                                       \
    "Warning: Non-standard license. This fx header is separate from the FastLED driver and carries different licensing terms. On the plus side, IT'S FUCKING AMAZING. ANIMartRIX: free for non-commercial use and licensed under the Creative Commons Attribution License CC BY-NC-SA 4.0. If you'd like to purchase a commercial use license please contact Stefan Petrick. Github: github.com/StefanPetrick/animartrix Reddit: reddit.com/user/StefanPetrick/ Modified by github.com/netmindz for class portability. Ported into FastLED by Zach Vorhies."
#endif 

#include "crgb.h"
#include "fl/force_inline.h"
#include "fl/compiler_control.h"

#include "bleControl.h"
//#include "../profiler.h"

using namespace fl;

// Math helpers ---------------------------------------------

#ifndef FL_ANIMARTRIX_USES_FAST_MATH
    #define FL_ANIMARTRIX_USES_FAST_MATH 1
#endif

// Wrapper functions that take radians and return float (-1.0 to 1.0)
// Using FastLED's sin32/cos32 approximations for better performance
inline float sin_fast(float angle_radians) {
    // Convert radians to sin32 units: 16777216 / (2*PI) = 2671177.0f
    uint32_t angle_sin32 = (uint32_t)(angle_radians * 2671177.0f);
    // sin32 returns -2147418112 to 2147418112, convert to -1.0 to 1.0
    return fl::sin32(angle_sin32) / 2147418112.0f;
}

inline float cos_fast(float angle_radians) {
    // Convert radians to cos32 units: 16777216 / (2*PI) = 2671177.0f
    uint32_t angle_cos32 = (uint32_t)(angle_radians * 2671177.0f);
    // cos32 returns -2147418112 to 2147418112, convert to -1.0 to 1.0
    return fl::cos32(angle_cos32) / 2147418112.0f;
}

#define FL_SIN_F(x) sin_fast(x)
#define FL_COS_F(x) cos_fast(x)

#if FL_ANIMARTRIX_USES_FAST_MATH
    FL_FAST_MATH_BEGIN
    FL_OPTIMIZATION_LEVEL_O3_BEGIN
#endif

#ifndef PI
    #define PI 3.1415926535897932384626433832795
#endif

// ----------------------------------------------------------

namespace animartrix_detail {

    // Audio elements ------------------------------------

    uint32_t cTimestamp = 0;
    float cRmsNorm = 0.0f;
    float cRmsFactor = 0.0f;
    //float cVocalConfidence = 0.0f;
    float voxLevel = 0.0f;
    myAudio::Bus cBusA;
    myAudio::Bus cBusB;
    myAudio::Bus cBusC;
    
    inline void getAudio(myAudio::binConfig& b) {
  
        const myAudio::AudioFrame& frame = myAudio::updateAudioFrame(b);

        cRmsNorm = frame.valid ? frame.rms_norm : 0.0f;
        cRmsFactor = frame.valid ? frame.rms_factor : 0.0f;
        cTimestamp = frame.valid ? frame.timestamp : 0;
        cVocalConfidence = frame.valid ? frame.vocalConfidence : 0;

        if (frame.valid) {
            cBusA = frame.busA;
            cBusB = frame.busB;
            cBusC = frame.busC;
        }
        
        /*EVERY_N_SECONDS(2) {
            myAudio::printDiagnostics();
        }
    
        EVERY_N_SECONDS(10) {
            myAudio::printBusSettings();
        }*/
    
    }

    struct render_parameters {
        float center_x = (999 / 2) - 0.5; // center of the matrix
        float center_y = (999 / 2) - 0.5;
        float dist, angle;
        float scale_x = 0.1f; // smaller values = zoom in
        float scale_y = 0.1f;
        float scale_z = 0.1f;
        float offset_x, offset_y, offset_z;
        float z;
        float low_limit = 0.0f; // getting contrast by raising the black point
        float high_limit = 1.0f;
    };

    #define num_oscillators 10

    struct oscillators {
        float master_speed; // global transition speed
        float offset[num_oscillators];  // oscillators can be shifted by a time offset
        float ratio[num_oscillators]; // speed ratios for the individual oscillators
    };

    struct modulators {
        float linear[num_oscillators];      // returns 0 to FLT_MAX
        float radial[num_oscillators];      // returns 0 to 2*PI
        float directional[num_oscillators]; // returns -1 to 1
        float noise_angle[num_oscillators]; // returns 0 to 2*PI
    };

    struct filters {
    // IDEAS FOR FUTURE IMPLEMENTATION
    // ripple
    // shimmer
    // blur??
    };

    struct rgb {
        float red, green, blue;
    };

    static const uint8_t PERLIN_NOISE[] = {
        151, 160, 137, 91,  90,  15,  131, 13,  201, 95,  96,  53,  194, 233, 7,
        225, 140, 36,  103, 30,  69,  142, 8,   99,  37,  240, 21,  10,  23,  190,
        6,   148, 247, 120, 234, 75,  0,   26,  197, 62,  94,  252, 219, 203, 117,
        35,  11,  32,  57,  177, 33,  88,  237, 149, 56,  87,  174, 20,  125, 136,
        171, 168, 68,  175, 74,  165, 71,  134, 139, 48,  27,  166, 77,  146, 158,
        231, 83,  111, 229, 122, 60,  211, 133, 230, 220, 105, 92,  41,  55,  46,
        245, 40,  244, 102, 143, 54,  65,  25,  63,  161, 1,   216, 80,  73,  209,
        76,  132, 187, 208, 89,  18,  169, 200, 196, 135, 130, 116, 188, 159, 86,
        164, 100, 109, 198, 173, 186, 3,   64,  52,  217, 226, 250, 124, 123, 5,
        202, 38,  147, 118, 126, 255, 82,  85,  212, 207, 206, 59,  227, 47,  16,
        58,  17,  182, 189, 28,  42,  223, 183, 170, 213, 119, 248, 152, 2,   44,
        154, 163, 70,  221, 153, 101, 155, 167, 43,  172, 9,   129, 22,  39,  253,
        19,  98,  108, 110, 79,  113, 224, 232, 178, 185, 112, 104, 218, 246, 97,
        228, 251, 34,  242, 193, 238, 210, 144, 12,  191, 179, 162, 241, 81,  51,
        145, 235, 249, 14,  239, 107, 49,  192, 214, 31,  181, 199, 106, 157, 184,
        84,  204, 176, 115, 121, 50,  45,  127, 4,   150, 254, 138, 236, 205, 93,
        222, 114, 67,  29,  24,  72,  243, 141, 128, 195, 78,  66,  215, 61,  156,
        180};

    FASTLED_FORCE_INLINE uint8_t P(uint8_t x) {
        const uint8_t idx = x & 255;
        const uint8_t *ptr = PERLIN_NOISE + idx;
        return *ptr;
    }

    // -----------------------------------------------------------
    
    class ANIMartRIX {

      public:
        int num_x; // how many LEDs are in one row?
        int num_y; // how many rows?

        float speed_factor = 1; // 0.1 to 10

        float radial_filter_radius = 23.0; // on 32x32, use 11 for 16x16
        float radialDimmer = 1.0f;
        float radialDimmer2 = 1.0f;
        float radialFilterFalloff = 1.0f;

        bool serpentine;

        render_parameters animation; // all animation parameters in one place
        oscillators timings;         // oscillator inputs; all time/speed settings in one place
        modulators move;            // oscillator outputs; all oscillator based movers and shifters at one place
        rgb pixel;
        filters filter;

        fl::vector<fl::vector<float>>
            polar_theta; // look-up table for polar angles
        fl::vector<fl::vector<float>>
            distance; // look-up table for polar distances

        float show1, show2, show3, show4, show5, show6, show7, show8, show9, show0;

        ANIMartRIX() {}

        ANIMartRIX(int w, int h) { this->init(w, h); }

        virtual ~ANIMartRIX() {}

        virtual uint16_t xyMap(uint16_t x, uint16_t y) = 0;

        uint32_t currentTime = 0;
        void setTime(uint32_t t) { currentTime = t; }
        uint32_t getTime() { return currentTime ? currentTime : fl::millis(); }

        void init(int w, int h) {
            animation = render_parameters();
            timings = oscillators();
            move = modulators();
            pixel = rgb();

            this->num_x = w;
            this->num_y = h;

            this->radial_filter_radius = std::min(w,h) * 0.65;
        
            // precalculate all polar coordinates; polar origin is set to matrix centre
            render_polar_lookup_table(
                (num_x / 2) - 0.5,
                (num_y / 2) - 0.5);  
            
            // Set default speed ratio for the oscillators. Not all effects set their own.
            timings.master_speed = 0.01;
        }

        /**
         * @brief Set the Speed Factor 0.1 to 10 - 1 for original speed
         *
         * @param speed
         */
        void setSpeedFactor(float speed) { this->speed_factor = speed; }

        float radialFilterFactor(float radius, float distance, float falloff) {
            if (distance >= radius) return 0.0f;
            float factor = 1.0f - (distance / radius);
            return fl::powf(factor, falloff);
        }

        // Dynamic darkening methods *************************************

        float subtract(float &a, float &b) { return a - b; }

        float multiply(float &a, float &b) { return a * b / 255.f; }

        // makes low brightness darker
        // sets the black point high = more contrast
        // animation.low_limit should be 0 for best results
        float colorburn(float &a, float &b) {
            return (1 - ((1 - a / 255.f) / (b / 255.f))) * 255.f;
        }

        // Dynamic brightening methods **********************************

        float add(float &a, float &b) { return a + b; }

        // makes bright even brighter
        // reduces contrast
        float screen(float &a, float &b) {
            return (1 - (1 - a / 255.f) * (1 - b / 255.f)) * 255.f;
        }

        float colordodge(float &a, float &b) { return (a / (255.f - b)) * 255.f; }

        //***************************************************************
            /* 
        Ken Perlins improved noise
        -  http://mrl.nyu.edu/~perlin/noise/
        C-port:  http://www.fundza.com/c4serious/noise/perlin/perlin.html
        by Malcolm Kesson;   arduino port by Peter Chiochetti, Sep 2007 :
        -  make permutation constant byte, obsoletes init(), lookup % 256
        */

        float fade(float t) { return t * t * t * (t * (t * 6 - 15) + 10); }
        float lerp(float t, float a, float b) { return a + t * (b - a); }
        float grad(int hash, float x, float y, float z) {
            int h = hash & 15;       /* CONVERT LO 4 BITS OF HASH CODE */
            float u = h < 8 ? x : y, /* INTO 12 GRADIENT DIRECTIONS.   */
                v = h < 4                ? y
                    : h == 12 || h == 14 ? x
                                        : z;
            return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
        }

        float pnoise(float x, float y, float z) {

            float fx = fl::floor(x);
            float fy = fl::floor(y);
            float fz = fl::floor(z);
            int X = (int)fx & 255;
            int Y = (int)fy & 255;
            int Z = (int)fz & 255;
            x -= fx;
            y -= fy;
            z -= fz;

            float u = fade(x), /* COMPUTE FADE CURVES */
                v = fade(y),   /* FOR EACH OF X,Y,Z.  */
                w = fade(z);
            int A = P(X) + Y, AA = P(A) + Z,
                AB = P(A + 1) + Z, /* HASH COORDINATES OF */
                B = P(X + 1) + Y, BA = P(B) + Z,
                BB = P(B + 1) + Z; /* THE 8 CUBE CORNERS, */

            // OPTIMIZATION: Pre-calculate offset values used in grad()
            float x1 = x - 1;
            float y1 = y - 1;
            float z1 = z - 1;

            return lerp(w,
                        lerp(v,
                            lerp(u, grad(P(AA), x, y, z),        /* AND ADD */
                                grad(P(BA), x1, y, z)),         /* BLENDED */
                            lerp(u, grad(P(AB), x, y1, z),       /* RESULTS */
                                grad(P(BB), x1, y1, z))),       /* FROM  8 */
                        lerp(v,
                            lerp(u, grad(P(AA + 1), x, y, z1),      /* CORNERS */
                                grad(P(BA + 1), x1, y, z1)),       /* OF CUBE */
                            lerp(u, grad(P(AB + 1), x, y1, z1),
                                grad(P(BB + 1), x1, y1, z1))));
        }

        //***************************************************************

        void calculate_oscillators(oscillators &timings) {

            // global animation speed
            float runtime = getTime() * timings.master_speed * speed_factor; // runtime was double

            for (uint8_t i = 0; i < num_oscillators; i++) {

                // continously rising offsets, returns 0 to max_float
                move.linear[i] = 
                    (runtime + timings.offset[i]) * timings.ratio[i];

                // angle offsets for continous rotation, returns 0 to 2 * PI
                move.radial[i] = 
                    fl::fmodf(move.linear[i], 2 * PI); 

                // directional offsets or factors, returns -1 to 1
                move.directional[i] = 
                    FL_SIN_F(move.radial[i]); 

                // noise based angle offset, returns 0 to 2 * PI
                move.noise_angle[i] =
                    PI * (1 + pnoise(move.linear[i], 0, 0));
            
            }
        }
    
        //***************************************************************
        void run_default_oscillators(float master_speed ) {
                timings.master_speed = master_speed;

                // speed ratios for the oscillators, higher values = faster transitions
                timings.ratio[0] = 1; 
                timings.ratio[1] = 2;
                timings.ratio[2] = 3;
                timings.ratio[3] = 4;
                timings.ratio[4] = 5;
                timings.ratio[5] = 6;
                timings.ratio[6] = 7;
                timings.ratio[7] = 8;
                timings.ratio[8] = 9;
                timings.ratio[9] = 10;

                timings.offset[0] = 000;
                timings.offset[1] = 100;
                timings.offset[2] = 200;
                timings.offset[3] = 300;
                timings.offset[4] = 400;
                timings.offset[5] = 500;
                timings.offset[6] = 600;
                timings.offset[7] = 700;
                timings.offset[8] = 800;
                timings.offset[9] = 900;

                calculate_oscillators(timings);
            }

        //***************************************************************

        // Convert the 2 polar coordinates back to cartesian ones & also apply all
        // 3d transitions. Calculate the noise value at this point based on the 5
        // dimensional manipulation of the underlying coordinates.

        float render_value(render_parameters &animation) {

            // convert polar coordinates back to cartesian ones
            // FL_SIN_F and FL_COS_F now use sin_fast/cos_fast wrappers with sin32/cos32

            float newx = (animation.offset_x + animation.center_x -
                        (FL_COS_F(animation.angle) * animation.dist)) *
                        animation.scale_x;
            float newy = (animation.offset_y + animation.center_y -
                        (FL_SIN_F(animation.angle) * animation.dist)) *
                        animation.scale_y;
            float newz = (animation.offset_z + animation.z) * animation.scale_z;

            // render noisevalue at this new cartesian point
            float raw_noise_field_value = pnoise(newx, newy, newz);

            // A) enhance histogram (improve contrast) by setting the black and
            // white point (low & high_limit) B) scale the result to a 0-255 range
            // (assuming you want 8 bit color depth per rgb chanel) Here happens the
            // contrast boosting & the brightness mapping

            if (raw_noise_field_value < animation.low_limit)
                raw_noise_field_value = animation.low_limit;
            if (raw_noise_field_value > animation.high_limit)
                raw_noise_field_value = animation.high_limit;

            float scaled_noise_value =
                map_float(raw_noise_field_value, animation.low_limit,
                        animation.high_limit, 0, 255);

            return scaled_noise_value;
        }

        // given a static polar origin we can precalculate the polar coordinates
        void render_polar_lookup_table(float cx, float cy) {

            polar_theta.resize(num_x, fl::vector<float>(num_y, 0.0f));
            distance.resize(num_x, fl::vector<float>(num_y, 0.0f));

            for (int xx = 0; xx < num_x; xx++) {
                for (int yy = 0; yy < num_y; yy++) {

                    float dx = xx - cx;
                    float dy = yy - cy;

                    distance[xx][yy] = fl::hypotf(dx, dy);
                    polar_theta[xx][yy] = fl::atan2f(dy, dx);
                }
            }
        }

        // float mapping maintaining 32 bit precision
        // we keep values with high resolution for potential later usage
        float map_float(float x, float in_min, float in_max, float out_min,
                        float out_max) {

            float result =
                (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
            if (result < out_min)
                result = out_min;
            if (result > out_max)
                result = out_max;

            return result;
        }

        // Avoid any possible color flicker by forcing the raw RGB values to be
        // 0-255. This enables to play freely with random equations for the
        // colormapping without causing flicker by accidentally missing the valid
        // target range.
        rgb rgb_sanity_check(rgb &pixel) {

            // Can never be negative color
            if (pixel.red < 0)
                pixel.red = 0;
            if (pixel.green < 0)
                pixel.green = 0;
            if (pixel.blue < 0)
                pixel.blue = 0;

            // discard everything above the valid 8 bit colordepth 0-255 range
            if (pixel.red > 255)
                pixel.red = 255;
            if (pixel.green > 255)
                pixel.green = 255;
            if (pixel.blue > 255)
                pixel.blue = 255;

            return pixel;
        }

        virtual void setPixelColorInternal(int x, int y, rgb pixel) = 0;


        //********************************************************************************************************************
        // EFFECTS ***********************************************************************************************************

        void Polar_Waves() {

            timings.master_speed = 0.5 * cSpeed;

            timings.ratio[0] = 0.0025 + cRatBase/100; 
            timings.ratio[1] = 0.0027 + cRatBase/100 * cRatDiff;
            timings.ratio[2] = 0.0031 + cRatBase/100 * 2 * cRatDiff;

            calculate_oscillators(timings);

            for (int x = 0; x < num_x; x++) {
                for (int y = 0; y < num_y; y++) {

                    animation.dist = distance[x][y] * cZoom;
                    animation.angle =
                        polar_theta[x][y] * cAngle
                        - animation.dist * 0.1
                        + move.radial[0];
                        // can add noise_angle for non-periodic rotation
                        // add multiple noise_angle for additional variation
                    animation.z = ((animation.dist * 1.5) - 10 * move.linear[0]) * cZ;
                    animation.scale_x = 0.15 * cScale;
                    animation.scale_y = 0.15 * cScale;
                    animation.offset_x = move.linear[0];
                    show1 = { Layer1 ? render_value(animation) : 0};
                    
                    animation.angle =
                        polar_theta[x][y] * cAngle
                        - animation.dist * 0.1 * cTwist
                        + move.radial[1];
                    animation.z = ((animation.dist * 1.5) - 10 * move.linear[1]) * cZ;
                    animation.offset_x = move.linear[1];
                    show2 = { Layer2 ? render_value(animation) : 0 };

                    animation.angle =
                        polar_theta[x][y] * cAngle
                        - animation.dist * 0.1 * cTwist
                        + move.radial[2];
                    animation.z = ((animation.dist * 1.5) - 10 * move.linear[2]) * cZ;
                    animation.offset_x = move.linear[2];
                    show3 = { Layer3 ? render_value(animation) : 0 };
                    
                    //float radial = (radius - distance[x][y]) / distance[x][y];
                    //float radialFilter = (radius - distance[x][y]) / distance[x][y];
                    
                    float radius = radial_filter_radius * cRadius; // * cRms;
                    radialFilterFalloff = cEdge;
                    radialDimmer = radialFilterFactor(radius, distance[x][y], radialFilterFalloff);
                
                    pixel.red = show1 * cRed * radialDimmer; 
                    pixel.green = show2 * cGreen * radialDimmer; 
                    pixel.blue = show3 * cBlue * radialDimmer;

                    pixel = rgb_sanity_check(pixel);
                    setPixelColorInternal(x, y, pixel);

                }
            }
        }

        //*******************************************************************************

        void Spiralus() {

            timings.master_speed = 0.0011 * cSpeed ; // * bpmFactor
            
            timings.ratio[0] = 1.5 + cRatBase * 2 * cRatDiff;       
            timings.ratio[1] = 2.3 + cRatBase * 2 * cRatDiff;
            timings.ratio[2] = 3 + cRatBase * 2 * cRatDiff;
            timings.ratio[3] = 0.05 + cRatBase/10 ;
            timings.ratio[4] = 0.2 + cRatBase/10 ;
            timings.ratio[5] = 0.03 + cRatBase/10 ;
            timings.ratio[6] = 0.025 + cRatBase/10 ;
            timings.ratio[7] = 0.021 + cRatBase/10 ;
            timings.ratio[8] = 0.027 + cRatBase/10 ;
            
            timings.offset[0] = 0 ;
            timings.offset[1] = 100 * cOffBase;
            timings.offset[2] = 200 * cOffBase * cOffDiff;
            timings.offset[3] = 300 * cOffBase * 1.25 * cOffDiff;
            timings.offset[4] = 400 * cOffBase * 1.5 * cOffDiff;
            timings.offset[5] = 500 * cOffBase * 1.75 * cOffDiff;
            timings.offset[6] = 600 * cOffBase * 2 * cOffDiff;

            calculate_oscillators(timings); 

            for (int x = 0; x < num_x; x++) {
                for (int y = 0; y < num_y; y++) {

                    animation.dist = distance[x][y] * cZoom;
                    animation.angle = 
                        2 * polar_theta[x][y] * cAngle  
                        + move.noise_angle[5] 
                        + move.directional[3] * move.noise_angle[6] * animation.dist / 10 * cTwist;
                    animation.scale_x = 0.08 * cScale;
                    animation.scale_y = 0.08 * cScale;
                    animation.scale_z = 0.02; 
                    animation.offset_y = -move.linear[0];
                    animation.offset_x = 0;
                    animation.offset_z = 0;
                    animation.z = move.linear[1] * cZ;
                    show1 = { Layer1 ? render_value(animation) : 0};

                    animation.angle = 
                        2 * polar_theta[x][y] * cAngle
                        + move.noise_angle[7]
                        + move.directional[5] * move.noise_angle[8] * animation.dist / 10 * cTwist;
                    animation.offset_y = -move.linear[1];
                    animation.z = move.linear[2] * cZ;
                    show2 = { Layer2 ? render_value(animation) : 0};

                    animation.angle = 
                        2 * polar_theta[x][y] * cAngle
                        + move.noise_angle[6] 
                        + move.directional[6] * move.noise_angle[7] * animation.dist / 10 * cTwist;
                    animation.offset_y = move.linear[2];
                    animation.z = move.linear[0] * cZ;
                    show3 = { Layer3 ? render_value(animation) : 0};

                    float radius = radial_filter_radius * cRadius;
                    radialFilterFalloff = cEdge;
                    //radialDimmer = radialFilterFactor(radius, distance[x][y], radialFilterFalloff);
                    radialDimmer = 1;   

                    pixel.red =     (show1 + show2) * cRed * radialDimmer;
                    pixel.green =   (show1 - show2) * cGreen * radialDimmer;
                    pixel.blue =    (show3 - show1) * cBlue * radialDimmer;

                    pixel = rgb_sanity_check(pixel);
                    setPixelColorInternal(x, y, pixel);
                }
            }
        }
    
        //*******************************************************************************

        void Caleido1() {

            timings.master_speed = 0.003 * cSpeed;
            
            timings.ratio[0] = 0.02 + cRatBase/10  ;
            timings.ratio[1] = 0.03 + cRatBase/10 * cRatDiff;
            timings.ratio[2] = 0.04 + cRatBase/10 * 1.5 * cRatDiff;
            timings.ratio[3] = 0.05 + cRatBase/10 * 2 * cRatDiff;
            timings.ratio[4] = 0.6 + cRatBase/5 ;
            
            timings.offset[0] = 0;
            timings.offset[1] = 100 * cOffBase;
            timings.offset[2] = 200 * cOffBase * cOffDiff;
            timings.offset[3] = 300 * cOffBase * 1.25 * cOffDiff;
            timings.offset[4] = 400 * cOffBase * 1.5 * cOffDiff;

            calculate_oscillators(timings); 

            for (int x = 0; x < num_x; x++) {
                for (int y = 0; y < num_y; y++) {

                    animation.dist = distance[x][y] * cZoom * (2 + move.directional[0]) / 3;
                    animation.angle = 
                        3 * polar_theta[x][y] * cAngle
                        + 3 * move.noise_angle[0] 
                        + move.radial[4];
                    animation.scale_x = 0.1 * cScale;
                    animation.scale_y = 0.1 * cScale;
                    animation.scale_z = 0.1;
                    animation.offset_y = 2 * move.linear[0];
                    animation.offset_x = 0;
                    animation.offset_z = 0;
                    animation.z = move.linear[0] * cZ;
                    show1 = { Layer1 ? render_value(animation) : 0};

                    animation.dist = distance[x][y] * cZoom * (2 + move.directional[1]) / 3;
                    animation.angle = 
                        4 * polar_theta[x][y] * cAngle
                        + 3 * move.noise_angle[1] 
                        + move.radial[4];
                    animation.offset_x = 2 * move.linear[1];
                    animation.z = move.linear[1] * cZ;
                    show2 = { Layer2 ? render_value(animation) : 0};

                    animation.dist = distance[x][y] * cZoom * (2 + move.directional[2]) / 3;
                    animation.angle = 
                        5 * polar_theta[x][y] * cAngle
                        + 3 * move.noise_angle[2]
                        + move.radial[4];
                    animation.offset_y = 2 * move.linear[2];
                    animation.z = move.linear[2] * cZ;
                    show3 = { Layer3 ? render_value(animation) : 0};

                    animation.dist = distance[x][y] * cZoom * (2 + move.directional[3]) / 3;
                    animation.angle = 
                        4 * polar_theta[x][y] * cAngle
                        + 3 * move.noise_angle[3]
                        + move.radial[4];
                    animation.offset_x = 2 * move.linear[3];
                    animation.z = move.linear[3] * cZ;
                    show4 = { Layer4 ? render_value(animation) : 0};

                    pixel.red = show1 * cRed;
                    pixel.green = (show3 * distance[x][y] / 10) * cGreen;
                    pixel.blue = ((show2 + show4) / 2) * cBlue;

                    pixel = rgb_sanity_check(pixel);

                    setPixelColorInternal(x, y, pixel);
                }
            }
        }

        //*******************************************************************************

        void Cool_Waves() {

            timings.master_speed = 0.01 * cSpeed; 
            
            timings.ratio[0] = 2  + cRatBase; 
            timings.ratio[1] = 2.1 + cRatBase * cRatDiff;
            timings.ratio[2] = 1.2 + cRatBase * cRatDiff;;

            timings.offset[1] = 100 * cOffBase;
            timings.offset[2] = 200 * cOffBase * cOffDiff;
            timings.offset[3] = 300 * cOffBase * 1.5 * cOffDiff;

            calculate_oscillators(timings);

            for (int x = 0; x < num_x; x++) {
                for (int y = 0; y < num_y; y++) {

                    animation.angle = polar_theta[x][y] *cAngle;
                    animation.scale_x = 0.1 * cScale;
                    animation.scale_y = 0.1 * cScale;
                    animation.scale_z = 0.1;
                    animation.dist = distance[x][y] * cZoom;
                    animation.offset_y = 0;
                    animation.offset_x = 0;
                    animation.z = (2 * distance[x][y] - move.linear[0]) * cZ;
                    show1 = { Layer1 ? render_value(animation) : 0};

                    animation.angle = polar_theta[x][y];
                    animation.z = (2 * distance[x][y] - move.linear[1]) * cZ;
                    show2 = { Layer2 ? render_value(animation) : 0};

                    pixel.red = show1;
                    pixel.green = 0;
                    pixel.blue = show2;

                    pixel = rgb_sanity_check(pixel);

                    setPixelColorInternal(x, y, pixel);
                }
            }
        }
    
        //*******************************************************************************

        void Chasing_Spirals() {

            timings.master_speed = 0.01 * cSpeed; 

            timings.ratio[0] = 0.1 +  cRatBase/10;
            timings.ratio[1] = 0.13 + cRatBase/10 * cRatDiff ;
            timings.ratio[2] = 0.16 + cRatBase/10 * 2 * cRatDiff;

            timings.offset[1] = 10 * cOffBase;
            timings.offset[2] = 20 * cOffBase * cOffDiff;
            timings.offset[3] = 30 * cOffBase * 2 * cOffDiff;

            calculate_oscillators(timings); 

            float Twister = cAngle * move.directional[0];

            for (int x = 0; x < num_x; x++) {
                for (int y = 0; y < num_y; y++) {

                    animation.dist = distance[x][y] / 4 * cZoom;
                    animation.angle =
                        3 * polar_theta[x][y] * cAngle
                        + move.radial[0] 
                        - distance[x][y]; // * Twister;
                    animation.scale_z = .1;
                    animation.scale_y = .1 * cScale;
                    animation.scale_x = .1 * cScale;
                    animation.offset_x = move.linear[0];
                    animation.offset_y = 0;
                    animation.offset_z = 0;
                    animation.z = 0;
                    show1 = { Layer1 ? render_value(animation) : 0};

                    animation.angle =
                        3 * polar_theta[x][y] * cAngle
                        + move.radial[1] 
                        - distance[x][y] * Twister;
                    animation.offset_x = move.linear[1];
                    show2 = { Layer2 ? render_value(animation) : 0};

                    animation.angle =
                        3 * polar_theta[x][y] * cAngle
                        + move.radial[2] 
                        - distance[x][y] * Twister;
                    animation.offset_x = move.linear[2];
                    show3 = { Layer3 ? render_value(animation) : 0};

                    float radius = radial_filter_radius * cRadius;
                    radialFilterFalloff = cEdge;
                    radialDimmer = radialFilterFactor(radius, distance[x][y], radialFilterFalloff);

                    pixel.red =     (3 * show1 * cRed) * radialDimmer;
                    pixel.green =   (show2 * cGreen) / 2 * radialDimmer;
                    pixel.blue =    (show3 * cBlue) / 4 * radialDimmer;

                    pixel = rgb_sanity_check(pixel);

                    setPixelColorInternal(x, y, pixel);
                }
            }
        }

        //*******************************************************************************

        void Complex_Kaleido_6() {

            //static bool freshRun = false;

            timings.master_speed = 0.01 * cSpeed; 

            timings.ratio[0] = 0.025 + cRatBase/10.f; 
            timings.ratio[1] = 0.027 + cRatBase/10.f * cRatDiff;
            timings.ratio[2] = 0.031 + cRatBase/10.f * 1.2f * cRatDiff;
            timings.ratio[3] = 0.033 + cRatBase/10.f * 1.4f * cRatDiff;
            timings.ratio[4] = 0.037 + cRatBase/10.f * 1.6f * cRatDiff;
            timings.ratio[5] = 0.038 + cRatBase/10.f * 1.8f * cRatDiff;
            timings.ratio[6] = 0.041 + cRatBase/10.f * 2.f * cRatDiff;
            timings.ratio[7] = 0.031 + 0.48f/10.f * 1.2f * 1.4f;
            timings.ratio[8] = 0.037 + 0.48f/10.f * 1.6f * 1.4f;
            
            if (audioEnabled){
                
                myAudio::binConfig& b = maxBins ? myAudio::bin32 : myAudio::bin16;
                getAudio(b);
                voxLevel = myAudio::vocalResponse();
            
                if (cBusA.isActive) {
                    /*if (freshRun) {
                        myAudio::busA.threshold = 0.25f;
                        myAudio::busA.peakBase = 1.0f;
                        myAudio::busA.rampAttack = 20.0f;
                        myAudio::busA.rampDecay = 40.0f;
                    }*/
                    myAudio::dynamicPulse(cBusA, cTimestamp);}
                
                if (cBusB.isActive) {
                    /*if (freshRun) {
                        myAudio::busB.threshold = 0.25f;
                        myAudio::busB.peakBase = 0.0f;
                        myAudio::busB.rampAttack = 40.0f;
                        myAudio::busB.rampDecay = 200.0f;
                    }*/
                    myAudio::dynamicPulse(cBusB, cTimestamp);
                }
                
                if (cBusC.isActive) {
                  /*if (freshRun) {
                        myAudio::busC.threshold = 0.6f;
                        myAudio::busC.peakBase = 0.5f;
                        myAudio::busC.rampAttack = 30.0f;
                        myAudio::busC.rampDecay = 100.0f;
                    }*/
                    myAudio::dynamicPulse(cBusC, cTimestamp);
                }

                //freshRun = false;

            } 

            calculate_oscillators(timings);
            
            float Twister = cAngle * move.directional[0] * cTwist*0.2f * (1.0f + cBusB.normEMA) ;

            for (int x = 0; x < num_x; x++) {
                for (int y = 0; y < num_y; y++) {

                    // OPTIMIZATION: Cache per-pixel calculations
                    float polar_theta_angle = polar_theta[x][y] * cAngle;
                    float dist_zoomed = distance[x][y] * cZoom;
                                     
                   // primarily mapped to blue as busA (bass) bus
                    animation.dist = dist_zoomed * 1.1f;
                    animation.angle = 
                        8.0f * polar_theta_angle 
                        + move.radial[0];
                    animation.z = 500.f * cZ;
                    animation.scale_x = 0.06f * cScale;
                    animation.scale_y = 0.06f * cScale; 
                    animation.offset_z = -10.f * move.linear[1];
                    animation.offset_y = 10.f * move.noise_angle[1];
                    animation.offset_x = 10.f * move.noise_angle[3];
                    show1 = { Layer1 ? render_value(animation) : 0};
                   
                    // primarily mapped to red as busB (middle) bus
                    animation.dist = dist_zoomed ; 
                    animation.angle = 
                        4.0f * polar_theta_angle
                        + 2.0f * move.radial[0] // 
                        - distance[x][y] * Twister * (1.0f + cBusB.normEMA*0.3f); // * move.noise_angle[5] 
                        //+ move.directional[3]; 
                    animation.z = 5.f * cZ;
                    animation.scale_x = 0.06f * cScale;
                    animation.scale_y = 0.06f * cScale; 
                    //animation.offset_z = -10.f * move.linear[0];
                    //animation.offset_y = 10.f * move.noise_angle[0];
                    //animation.offset_x = 10.f * move.noise_angle[4];
                    show2 = { Layer2 ? render_value(animation) : 0};
                    
                    // primarily mapped to green as busC (treble) bus
                    animation.dist = dist_zoomed;
                    animation.angle = 
                        4.0f * polar_theta_angle  
                        - move.radial[1];
                    animation.z = 25.f * cZ;
                    animation.scale_x = 0.132f * cScale;
                    animation.scale_y = 0.132f * cScale;
                    //animation.offset_z = -10.f * move.linear[7];
                    //animation.offset_y = 10.f * move.noise_angle[7];
                    //animation.offset_x = 10.f * move.noise_angle[8];
                    show3 = { Layer3 ? render_value(animation) : 0};

                    float radius = radial_filter_radius * cRadius;
                    float radiusB = radial_filter_radius * cRadius*0.7f * (1.0f + voxLevel); // * 1.7f
                    
                    radialDimmer = radialFilterFactor(radius, distance[x][y], cEdge);
                    float radialDimmerB = radialFilterFactor(radiusB, distance[x][y], cEdge*(1.0f + cBusB.avResponse*0.5f));
                    
                    pixel.blue = 0.8f*cBlue * (1.5f*show1 - show2 - show3) * cBusA.avResponse ; 
                    pixel.red = cRed * show2*2.0f * FL_MAX(radialDimmerB, 0.01f) * (0.5f + cBusB.normEMA*voxLevel);
                    pixel.green = 0.8f*cGreen * (0.5f*show3 - show2 - show1*.8f) * cBusC.avResponse*0.8;

                    pixel = rgb_sanity_check(pixel);

                    setPixelColorInternal(x, y, pixel);
                }
            }   
        }

        //*******************************************************************************

        void Water() {

            timings.master_speed = 0.037 * cSpeed;

            timings.ratio[0] = 0.025 + cRatBase/10; 
            timings.ratio[1] = 0.027 + cRatBase/10 * cRatDiff;
            timings.ratio[2] = 0.031 + cRatBase/10 * 1.25* cRatDiff;
            timings.ratio[3] = 0.033 + cRatBase/10 * 1.5 * cRatDiff; 
            timings.ratio[4] = 0.037 + cRatBase/10 * 1.75 * cRatDiff;
            timings.ratio[5] = 0.1 + cRatBase/5;
            timings.ratio[6] = 0.41 + cRatBase/5;

            calculate_oscillators(timings);

            for (int x = 0; x < num_x; x++) {
                for (int y = 0; y < num_y; y++) {

                    animation.dist =
                        distance[x][y] * cZoom +
                        4 * FL_SIN_F(move.directional[5] * PI ) +
                        4 * FL_COS_F(move.directional[6] * PI );
                    animation.angle = 1 * polar_theta[x][y] * cAngle ;
                    animation.z = 5 * cZ;
                    animation.scale_x = 0.06 * cScale;
                    animation.scale_y = 0.06 * cScale;
                    animation.offset_z = -10 * move.linear[0];
                    animation.offset_y = 10;
                    animation.offset_x = 10;
                    animation.low_limit = 0;
                    show1 = { Layer1 ? render_value(animation) : 0};

                    animation.dist = 
                        (10 + move.directional[0]) * FL_SIN_F(-move.radial[5] + 
                        move.radial[0] + (distance[x][y] / (3)));
                    animation.angle = 1 * polar_theta[x][y] * cAngle ;
                    animation.z = 5 * cZ;
                    animation.scale_x = 0.1 * cScale;
                    animation.scale_y = 0.1 * cScale;
                    animation.offset_z = -10;
                    animation.offset_y = 20 * move.linear[0];
                    animation.offset_x = 10;
                    animation.low_limit = 0;
                    show2 = { Layer2 ? render_value(animation) : 0};

                    animation.dist = 
                        (10 + move.directional[1]) * FL_SIN_F(-move.radial[5] + 
                        move.radial[1] + (distance[x][y] / (3)));
                    animation.angle = 1 * polar_theta[x][y] * cAngle ;
                    animation.z = 500 * cZ;
                    animation.scale_x = 0.1 * cScale;
                    animation.scale_y = 0.1 * cScale;
                    animation.offset_z = -10;
                    animation.offset_y = 20 * move.linear[1];
                    animation.offset_x = 10;
                    animation.low_limit = 0;
                    show3 = { Layer3 ? render_value(animation) : 0};

                    animation.dist = 
                        (10 + move.directional[2]) * FL_SIN_F(-move.radial[5] + 
                        move.radial[2] + (distance[x][y] / (3)));
                    animation.angle = 1 * polar_theta[x][y] * cAngle ;
                    animation.z = 500 * cZ;
                    animation.scale_x = 0.1 * cScale;
                    animation.scale_y = 0.1 * cScale;
                    animation.offset_z = -10;
                    animation.offset_y = 20 * move.linear[2];
                    animation.offset_x = 10;
                    animation.low_limit = 0;
                    show4 = { Layer4 ? render_value(animation) : 0};

                    // float radius = radial_filter_radius;   // radius of a radial
                    // brightness filter float radial =
                    // (radius-distance[x][y])/distance[x][y];

                    // pixel.red    = show2;

                    pixel.blue = (0.7 * show2 + 0.6 * show3 + 0.5 * show4);
                    pixel.red = pixel.blue - 40;
                    // pixel.red     = radial*show3;
                    //pixel.green     = 0.9*show4;

                    pixel = rgb_sanity_check(pixel);

                    setPixelColorInternal(x, y, pixel);
                }
            }
        }

        //*******************************************************************************
        
        void Experiment1() { 

            timings.master_speed = 0.02 * cSpeed;

            timings.ratio[0] = 0.0025 + cRatBase/100 ; 
            timings.ratio[1] = 0.0027 + cRatBase/100 * 1.2 * cRatDiff;
            timings.ratio[2] = 0.0031 + cRatBase/100 * 1.4 * cRatDiff;
            timings.ratio[3] = 0.0033 + cRatBase/100 * 1.6 * cRatDiff; 
            timings.ratio[4] = 0.0036 + cRatBase/100 * 1.8 * cRatDiff;
            timings.ratio[5] = 0.0039 + cRatBase/100 * 2 * cRatDiff;

            calculate_oscillators(timings);

            for (int x = 0; x < num_x ; x++) {
                for (int y = 0; y < num_y ; y++) {

                    animation.dist = distance[x][y] * cZoom;
                    animation.angle = 
                        polar_theta[x][y] * cAngle 
                        + 5 * move.noise_angle[0];
                    animation.z = 5 * cZ;
                    animation.scale_x = 0.1 * cScale;
                    animation.scale_y = 0.1 * cScale;
                    animation.offset_z = 50 * move.linear[0];
                    animation.offset_x = 150 * move.directional[0];
                    animation.offset_y = 150 * move.directional[1];
                    show1 = { Layer1 ? render_value(animation) : 0};

                    animation.angle = 
                        polar_theta[x][y] * cAngle 
                        + 4 * move.noise_angle[1];
                    animation.z = 15 * cZ;
                    animation.scale_x = 0.15 * cScale;
                    animation.scale_y = 0.15 * cScale;
                    animation.offset_z = 50 * move.linear[1];
                    animation.offset_x = 150 * move.directional[1];
                    animation.offset_y = 150 * move.directional[2];
                    show2 = { Layer2 ? render_value(animation) : 0};

                    animation.angle = 
                        polar_theta[x][y] * cAngle 
                        + 5 * move.noise_angle[2];
                    animation.z = 25 * cZ;
                    animation.scale_x = 0.1 * cScale;
                    animation.scale_y = 0.1 * cScale;
                    animation.offset_z = 50 * move.linear[2];
                    animation.offset_x = 150 * move.directional[2];
                    animation.offset_y = 150 * move.directional[3];
                    show3 = { Layer3 ? render_value(animation) : 0};

                    animation.angle = 
                        polar_theta[x][y] * cAngle 
                        + 5 * move.noise_angle[3];
                    animation.z = 35 * cZ;
                    animation.scale_x = 0.15 * cScale;
                    animation.scale_y = 0.15 * cScale;
                    animation.offset_z = 50 * move.linear[3];
                    animation.offset_x = 150 * move.directional[3];
                    animation.offset_y = 150 * move.directional[4];
                    show4 = { Layer4 ? render_value(animation) : 0};

                    animation.angle = 
                        polar_theta[x][y] * cAngle 
                        + 5 * move.noise_angle[4];
                    animation.z = 45 * cZ;
                    animation.scale_x = 0.2 * cScale;
                    animation.scale_y = 0.2 * cScale;
                    animation.offset_z = 50 * move.linear[4];
                    animation.offset_x = 150 * move.directional[4];
                    animation.offset_y = 150 * move.directional[5];
                    show5 = { Layer5 ? render_value(animation) : 0};

                    //show6 = screen(show1, show2);
                    //show7 = colordodge(show3, show4);
                    //show8 = multiply(show5, show7);
                    
                    pixel.red = (show1 + show2) * cRed;
                    pixel.green = (show3 + show4) * cGreen;
                    pixel.blue = show5 * cBlue;

                    //pixel.red = (show4/5 + show6) * cRed;
                    //pixel.green = (show5/5 + show7) * cGreen;
                    //pixel.blue = show8 * cBlue;

                    pixel = rgb_sanity_check(pixel);
                    setPixelColorInternal(x, y, pixel);

                }
            }
        }

        //*******************************************************************************

        void Experiment2() {

            timings.master_speed = 0.01 * cSpeed; 

            timings.ratio[0] = 0.01;
            timings.ratio[1] = 0.02;
            timings.ratio[2] = 0.03;
            timings.ratio[3] = 0.04;
            timings.ratio[4] = 0.05; 
            
            timings.offset[0] = 0;
            timings.offset[1] = 100 ;
            timings.offset[2] = 200 ;
            timings.offset[3] = 300 ;
            timings.offset[4] = 400 ;

            myAudio::binConfig& b = maxBins ? myAudio::bin32 : myAudio::bin16;
            getAudio(b);
            
            calculate_oscillators(timings);

            for (int x = 0; x < num_x; x++) {
                for (int y = 0; y < num_y; y++) {

                    animation.dist = distance[x][y];
                    animation.angle = polar_theta[x][y] * cAngle;
                    show1 = { Layer1 ? render_value(animation) : 0};

                    animation.dist =
                        1 + distance[x][y] 
                        - move.radial[4];
                    animation.angle = 
                        polar_theta[x][y] * cAngle
                        + move.noise_angle[1];
                    show2 = { Layer2 ? render_value(animation) : 0};

                    animation.dist =
                        2 + distance[x][y]
                        - move.radial[5];
                    animation.angle = 
                        polar_theta[x][y] * cAngle 
                        + move.noise_angle[2]; 
                    show3 = { Layer3 ? render_value(animation) : 0};

                    //show4 = colordodge(show1, show2);

                    pixel.red    = show1 ;
                    pixel.green  = show2 ;
                    pixel.blue   = show3 ;

                    pixel = rgb_sanity_check(pixel);

                    setPixelColorInternal(x, y, pixel);

                }
            }
        }

        //*******************************************************************************

        void Fluffy_Blobs() {

            timings.master_speed = 0.015 * cSpeed;  // master speed dial for everything
            float size = 0.15;             // size of the blobs - think of it as a global zoom factor
            
            timings.ratio[0] = 0.025 + cRatBase/10 * cRatDiff;      // set up 9 oscillators and detune their frequencies slighly
            timings.ratio[1] = 0.026 + cRatBase/10 * cRatDiff * 1.05;
            timings.ratio[2] = 0.027 + cRatBase/10 * cRatDiff * 1.1;
            timings.ratio[3] = 0.028 + cRatBase/10 * cRatDiff * 1.15;
            timings.ratio[4] = 0.029 + cRatBase/10 * cRatDiff * 1.2;
            timings.ratio[5] = 0.030 + cRatBase/10 * cRatDiff * 1.25;
            timings.ratio[6] = 0.031 + cRatBase/10 * cRatDiff * 1.3;
            timings.ratio[7] = 0.032 + cRatBase/10 * cRatDiff * 1.35;
            timings.ratio[8] = 0.033 + cRatBase/10 * cRatDiff * 1.4;

            calculate_oscillators(timings);
    
            // OPTIMIZATION: Pre-calculate per-frame values (used for all pixels)
            float radial_scaled[9];
            float linear_scaled[9];
            for (int i = 0; i < 9; i++) {
                radial_scaled[i] = cRadialSpeed * move.radial[i];
                linear_scaled[i] = cLinearSpeed * move.linear[i];
            }

            for (int x = 0; x < num_x; x++) {
                for (int y = 0; y < num_y; y++) {

                // render 9 layers with the same effect at slighliy different speeds and sizes

                // OPTIMIZATION: Cache per-pixel calculations
                float polar_theta_angle = polar_theta[x][y] * cAngle;
                float dist_zoomed = distance[x][y] * cZoom;

                // Layer1
                /* Includes parameters that are used in subsequent layers if unchanged;
                    so toggling Layer1 only affects display, not processing. Toggling other layers
                    determines whether they even process. */
                //animation.dist = distance[x][y] * cZoom ;
                //animation.angle = polar_theta[x][y] * cAngle + (cRadialSpeed * move.radial[0]);
                animation.dist = dist_zoomed;
                animation.angle = polar_theta_angle + radial_scaled[0];
                animation.z = 5;
                animation.scale_x = size * cScale;
                animation.scale_y = size * cScale;
                animation.offset_z = 0;
                //animation.offset_y = cLinearSpeed * move.linear[0];
                animation.offset_y = linear_scaled[0];
                animation.low_limit = 0;
                animation.high_limit = 1;
    
                show1 = { Layer1 ? render_value(animation) : 0};
                
                if (Layer2) {
                    //animation.angle = polar_theta[x][y] * cAngle + (cRadialSpeed *  move.radial[1]);
                    //animation.offset_y = cLinearSpeed * move.linear[1];
                    animation.angle = polar_theta_angle + radial_scaled[1];
                    animation.offset_y = linear_scaled[1];
                    animation.offset_z = 200 * cZ;
                    animation.scale_x = size * 1.1 * cScale;
                    animation.scale_y = size * 1.1 * cScale;
                    show2 = render_value(animation);
                } else {show2 = 0;}
                
                if (Layer3) {
                    //animation.angle = polar_theta[x][y] * cAngle + (cRadialSpeed *  move.radial[2]);
                    //animation.offset_y = cLinearSpeed * move.linear[2];
                    animation.angle = polar_theta_angle + radial_scaled[2];
                    animation.offset_y = linear_scaled[2];
                    animation.offset_z = 400 * cZ;
                    animation.scale_x = size * 1.2 * cScale;
                    animation.scale_y = size * 1.2 * cScale;
                    show3 = render_value(animation);
                } else {show3 = 0;}

                if (Layer4) {
                    //animation.angle = polar_theta[x][y] * cAngle + (cRadialSpeed *  move.radial[3]);
                    //animation.offset_y = cLinearSpeed * move.linear[3];
                    animation.angle = polar_theta_angle + radial_scaled[3];
                    animation.offset_y = linear_scaled[3];
                    animation.offset_z = 600 * cZ;
                    animation.scale_x = size * cScale;
                    animation.scale_y = size * cScale;
                    show4 = render_value(animation);
                } else {show4 = 0;}
                
                if (Layer5) {
                    //animation.angle = polar_theta[x][y] * cAngle + (cRadialSpeed *  move.radial[4]);
                    //animation.offset_y = cLinearSpeed * move.linear[4];
                    animation.angle = polar_theta_angle + radial_scaled[4];
                    animation.offset_y = linear_scaled[4];
                    animation.offset_z = 800 * cZ;
                    animation.scale_x = size * 1.1 * cScale;
                    animation.scale_y = size * 1.1 * cScale;
                    show5 = render_value(animation);
                } else {show5 = 0;}
                
                if (Layer6) {
                    //animation.angle = polar_theta[x][y]  * cAngle + (cRadialSpeed *  move.radial[5]);
                    //animation.offset_y = cLinearSpeed * move.linear[5];
                    animation.angle = polar_theta_angle + radial_scaled[5];
                    animation.offset_y = linear_scaled[5];
                    animation.offset_z = 1800 * cZ;
                    animation.scale_x = size * 1.2 * cScale;
                    animation.scale_y = size * 1.2 * cScale;
                    show6 = render_value(animation);
                } else {show6 = 0;}
                
                if (Layer7) {
                    //animation.angle = polar_theta[x][y] * cAngle + (cRadialSpeed *  move.radial[6]);
                    //animation.offset_y = cLinearSpeed * move.linear[6];
                    animation.angle = polar_theta_angle + radial_scaled[6];
                    animation.offset_y = linear_scaled[6];
                    animation.offset_z = 2800 * cZ;
                    animation.scale_x = size * cScale;
                    animation.scale_y = size * cScale;
                    show7 = render_value(animation);
                } else {show7 = 0;}

                if (Layer8) {
                    //animation.angle = polar_theta[x][y] * cAngle + (cRadialSpeed *  move.radial[7]);
                    //animation.offset_y = cLinearSpeed  * move.linear[7];
                    animation.angle = polar_theta_angle + radial_scaled[7];
                    animation.offset_y = linear_scaled[7];
                    animation.offset_z = 3800 * cZ;
                    animation.scale_x = size * 1.1 * cScale;
                    animation.scale_y = size * 1.1 * cScale;
                    show8 = render_value(animation);
                } else {show8 = 0;}

                if (Layer9) {
                    //animation.angle = polar_theta[x][y] * cAngle + (cRadialSpeed *  move.radial[8]);
                    //animation.offset_y = cLinearSpeed  * move.linear[8];
                    animation.angle = polar_theta_angle + radial_scaled[8];
                    animation.offset_y = linear_scaled[8];
                    animation.offset_z = 4800 * cZ;
                    animation.scale_x = size * 1.2 * cScale;
                    animation.scale_y = size * 1.2 * cScale;
                    show9 = render_value(animation);
                } else {show9 = 0;}

                // the factors modulate the color mix and overall appearance of the animations

                pixel.red = ( (show1 + show2 + show3) + (show4 + show5 + show6)) * cRed;   // red is the sum of layer 1, 2, 3
                                                                                        // I also add layer 4, 5, 6 (which modulates green)
                                                                                        // in order to add orange/yellow to the mix
                pixel.green = (0.8 * (show4 + show5 + show6)) * cGreen;                           // green is the sum of layer 4, 5, 6
                pixel.blue =  (0.3 * (show7 + show8 + show9)) * cBlue;                           // blue is the sum of layer 7, 8, 9

                pixel = rgb_sanity_check(pixel);

                setPixelColorInternal(x, y, pixel);

                }
            }
        
        } // Fluffy_Blobs

    }; // class ANIMartRIX

} // namespace animartrix_detail

//*******************************************************************************

// AuroraPortal program wrapper (MODE-driven, no FxEngine)
namespace animartrix {

    bool animartrixInstance = false;

    class AnimartrixAdapter : public animartrix_detail::ANIMartRIX {
        public:
            explicit AnimartrixAdapter(const fl::XYMap &xyMap) : mXyMap(xyMap) {
                mXyMap.convertToLookUpTable();
                init(mXyMap.getWidth(), mXyMap.getHeight());
            }

            void setLeds(fl::CRGB *leds) { mLeds = leds; }
            void clearLeds() { mLeds = nullptr; }
            void setColorOrder(uint8_t orderIndex) {
                static const uint8_t ORDER_TABLE[6][3] = {
                    {0, 1, 2}, // RGB
                    {0, 2, 1}, // RBG
                    {1, 0, 2}, // GRB
                    {1, 2, 0}, // GBR
                    {2, 0, 1}, // BRG
                    {2, 1, 0}  // BGR
                };
                if (orderIndex > 5) {
                    orderIndex = 0;
                }
                order_b0 = ORDER_TABLE[orderIndex][0];
                order_b1 = ORDER_TABLE[orderIndex][1];
                order_b2 = ORDER_TABLE[orderIndex][2];
            }

            void render(uint8_t mode) {
                switch (mode) {
                    case 0: Polar_Waves(); break;
                    case 1: Spiralus(); break;
                    case 2: Caleido1(); break;
                    case 3: Cool_Waves(); break;
                    case 4: Chasing_Spirals(); break;
                    case 5: Complex_Kaleido_6(); break;
                    case 6: Water(); break;
                    case 7: Experiment1(); break;
                    case 8: Experiment2(); break;
                    case 9: Fluffy_Blobs(); break;
                    default: Fluffy_Blobs(); break;
                }
            }

            uint16_t xyMap(uint16_t x, uint16_t y) override {
                return mXyMap.mapToIndex(x, y);
            }

            void setPixelColorInternal(int x, int y, animartrix_detail::rgb pixel) override {
                
                if (!mLeds) { return; }
                
                const uint16_t idx = xyMap(x, y);
                const uint8_t r = static_cast<uint8_t>(pixel.red);
                const uint8_t g = static_cast<uint8_t>(pixel.green);
                const uint8_t b = static_cast<uint8_t>(pixel.blue);
                const uint8_t raw[3] = {r, g, b};
                mLeds[idx].raw[0] = raw[order_b0];
                mLeds[idx].raw[1] = raw[order_b1];
                mLeds[idx].raw[2] = raw[order_b2];
            }

            uint16_t total() const { return mXyMap.getTotal(); }
            uint16_t width() const { return mXyMap.getWidth(); }
            uint16_t height() const { return mXyMap.getHeight(); }

        private:
            fl::XYMap mXyMap;
            fl::CRGB *mLeds = nullptr;
            uint8_t order_b0 = 0;
            uint8_t order_b1 = 1;
            uint8_t order_b2 = 2;
    };

    static fl::scoped_ptr<AnimartrixAdapter> animFx;
    static int lastMode = -1;
    static int lastColorOrder = -1;

    void initAnimartrix(const fl::XYMap &xyMap) {
        animartrixInstance = true;
        animFx.reset(new AnimartrixAdapter(xyMap));
        lastMode = -1;
        lastColorOrder = -1;
    }

    void runAnimartrix() {
        if (!animFx) {
            return;
        }

        if (MODE != lastMode) {
            animFx->init(animFx->width(), animFx->height());
            lastMode = MODE;
        }

        if (cColOrd != lastColorOrder) {
            animFx->setColorOrder(cColOrd);
            lastColorOrder = cColOrd;
        }

        animFx->setLeds(leds);
        animFx->setTime(fl::millis());
        animFx->render(MODE);
        animFx->clearLeds();
    }

} // namespace animartrix

// End fast math optimizations
#if FL_ANIMARTRIX_USES_FAST_MATH
FL_OPTIMIZATION_LEVEL_O3_END
FL_FAST_MATH_END
#endif
