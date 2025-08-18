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

#include "fl/vector.h"
#include <math.h>
#include "fl/stdint.h"

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

#include "crgb.h"
#include "fl/force_inline.h"
#include "fl/namespace.h"
#include "fl/math.h"
#include "fl/compiler_control.h"

#include "bleControl.h"

#ifndef FL_ANIMARTRIX_USES_FAST_MATH
#define FL_ANIMARTRIX_USES_FAST_MATH 1
#endif

#define FL_SIN_F(x) sinf(x)
#define FL_COS_F(x) cosf(x)

#if FL_ANIMARTRIX_USES_FAST_MATH
FL_FAST_MATH_BEGIN
FL_OPTIMIZATION_LEVEL_O3_BEGIN
#endif

//#define FASTLED_ANIMARTRIX_LICENSING_AGREEMENT 1
// Setting this to 1 means you agree to the licensing terms of the ANIMartRIX
// library for non commercial use only.
#if defined(FASTLED_ANIMARTRIX_LICENSING_AGREEMENT) ||                         \
    (FASTLED_ANIMARTRIX_LICENSING_AGREEMENT != 0)
#warning                                                                       \
    "Warning: Non-standard license. This fx header is separate from the FastLED driver and carries different licensing terms. On the plus side, IT'S FUCKING AMAZING. ANIMartRIX: free for non-commercial use and licensed under the Creative Commons Attribution License CC BY-NC-SA 4.0. If you'd like to purchase a commercial use license please contact Stefan Petrick. Github: github.com/StefanPetrick/animartrix Reddit: reddit.com/user/StefanPetrick/ Modified by github.com/netmindz for class portability. Ported into FastLED by Zach Vorhies."
#endif //

#ifndef PI
#define PI 3.1415926535897932384626433832795
#endif

#ifdef ANIMARTRIX_PRINT_USES_SERIAL
#define ANIMARTRIX_PRINT_USES_SERIAL(S) Serial.print(S)
#else
#define ANIMARTRIX_PRINT(S) (void)(S)
#endif

#define num_oscillators 10

namespace animartrix_detail {
FASTLED_USING_NAMESPACE

struct render_parameters {
    float center_x = (999 / 2) - 0.5; // center of the matrix
    float center_y = (999 / 2) - 0.5;
    float dist, angle;
    float scale_x = .1; // smaller values = zoom in
    float scale_y = .1;
    float scale_z = .1;
    float offset_x, offset_y, offset_z;
    float z;
    float low_limit = 0; // getting contrast by raising the black point
    float high_limit = 1;
};

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

class ANIMartRIX {

  public:
    int num_x; // how many LEDs are in one row?
    int num_y; // how many rows?

    float speed_factor = 1; // 0.1 to 10

    float radial_filter_radius = 23.0; // on 32x32, use 11 for 16x16
    float radialDimmer = 1;
    float radialFilterFalloff = 1;

    bool serpentine;

    render_parameters animation; // all animation parameters in one place
    oscillators timings;         // all speed settings in one place
    modulators move; // all oscillator based movers and shifters at one place
    rgb pixel;
    filters filter;

    fl::HeapVector<fl::HeapVector<float>>
        polar_theta; // look-up table for polar angles
    fl::HeapVector<fl::HeapVector<float>>
        distance; // look-up table for polar distances

    //unsigned long a, b, c; // for time measurements

    float show1, show2, show3, show4, show5, show6, show7, show8, show9, show0;

    ANIMartRIX() {}

    ANIMartRIX(int w, int h) { this->init(w, h); }

    virtual ~ANIMartRIX() {}

    virtual uint16_t xyMap(uint16_t x, uint16_t y) = 0;

    uint32_t currentTime = 0;
    void setTime(uint32_t t) { currentTime = t; }
    uint32_t getTime() { return currentTime ? currentTime : millis(); }

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

    float radialFilterFactor( float radius, float distance, float falloff) {
        if (distance >= radius) return 0.0f;
        float factor = 1.0f - (distance / radius);
        return powf(factor, falloff);
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

        int X = (int)floorf(x) & 255, /* FIND UNIT CUBE THAT */
            Y = (int)floorf(y) & 255, /* CONTAINS POINT.     */
            Z = (int)floorf(z) & 255;
        x -= floorf(x); /* FIND RELATIVE X,Y,Z */
        y -= floorf(y); /* OF POINT IN CUBE.   */
        z -= floorf(z);
        float u = fade(x), /* COMPUTE FADE CURVES */
            v = fade(y),   /* FOR EACH OF X,Y,Z.  */
            w = fade(z);
        int A = P(X) + Y, AA = P(A) + Z,
            AB = P(A + 1) + Z, /* HASH COORDINATES OF */
            B = P(X + 1) + Y, BA = P(B) + Z,
            BB = P(B + 1) + Z; /* THE 8 CUBE CORNERS, */

        return lerp(w,
                    lerp(v,
                         lerp(u, grad(P(AA), x, y, z),        /* AND ADD */
                              grad(P(BA), x - 1, y, z)),      /* BLENDED */
                         lerp(u, grad(P(AB), x, y - 1, z),    /* RESULTS */
                              grad(P(BB), x - 1, y - 1, z))), /* FROM  8 */
                    lerp(v,
                         lerp(u, grad(P(AA + 1), x, y, z - 1),   /* CORNERS */
                              grad(P(BA + 1), x - 1, y, z - 1)), /* OF CUBE */
                         lerp(u, grad(P(AB + 1), x, y - 1, z - 1),
                              grad(P(BB + 1), x - 1, y - 1, z - 1))));
    }

    //***************************************************************

    void calculate_oscillators(oscillators &timings) {

        // global animation speed
        double runtime = getTime() * timings.master_speed * speed_factor; 

        for (int i = 0; i < num_oscillators; i++) {

            // continously rising offsets, returns 0 to max_float
            move.linear[i] = 
                (runtime + timings.offset[i]) * timings.ratio[i];

            // angle offsets for continous rotation, returns 0 to 2 * PI
            move.radial[i] = 
                fmodf(move.linear[i], 2 * PI); 

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
    // dimensional manipulation of the underlaying coordinates.

    float render_value(render_parameters &animation) {

        // convert polar coordinates back to cartesian ones

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

        polar_theta.resize(num_x, fl::HeapVector<float>(num_y, 0.0f));
        distance.resize(num_x, fl::HeapVector<float>(num_y, 0.0f));

        for (int xx = 0; xx < num_x; xx++) {
            for (int yy = 0; yy < num_y; yy++) {

                float dx = xx - cx;
                float dy = yy - cy;

                distance[xx][yy] = hypotf(dx, dy);
                polar_theta[xx][yy] = atan2f(dy, dx);
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

        // Can never be negative colour
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
                
                float radius = radial_filter_radius * cRadius;
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

        timings.master_speed = 0.0011 * cSpeed;
        
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

    void Waves() {

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

                //float radius = radial_filter_radius + cRadius;
                //float radial_filter = (radius + cEdge - distance[x][y]) / ( radius + cEdge);

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

        timings.master_speed = 0.01 * cSpeed; 

        timings.ratio[0] = 0.025 + cRatBase/10; 
        timings.ratio[1] = 0.027 + cRatBase/10 * cRatDiff;
        timings.ratio[2] = 0.031 + cRatBase/10 * 1.2 * cRatDiff;
        timings.ratio[3] = 0.033 + cRatBase/10 * 1.4 * cRatDiff;
        timings.ratio[4] = 0.037 + cRatBase/10 * 1.6 * cRatDiff;
        timings.ratio[5] = 0.038 + cRatBase/10 * 1.8 * cRatDiff;
        timings.ratio[6] = 0.041 + cRatBase/10 * 2 * cRatDiff;

        calculate_oscillators(timings);

        float Twister = cAngle * move.directional[0] / 10;

        for (int x = 0; x < num_x; x++) {
            for (int y = 0; y < num_y; y++) {

                animation.dist = distance[x][y] * cZoom;
                animation.angle = 
                    4 * polar_theta[x][y] * cAngle 
                    + 16 * move.radial[0]
                    - distance[x][y] * Twister * move.noise_angle[5] 
                    + move.directional[3]; 
                animation.z = 5 * cZ;
                animation.scale_x = 0.06 * cScale;
                animation.scale_y = 0.06 * cScale;
                animation.offset_z = -10 * move.linear[0];
                animation.offset_y = 10 * move.noise_angle[0];
                animation.offset_x = 10 * move.noise_angle[4];
                animation.low_limit = 0;
                show1 = { Layer1 ? render_value(animation) : 0};

                animation.angle = 
                    16 * polar_theta[x][y] * cAngle
                    + 16 * move.radial[1];
                animation.z = 500 * cZ;
                animation.scale_x = 0.06 * cScale;;
                animation.scale_y = 0.06 * cScale;;
                animation.offset_z = -10 * move.linear[1];
                animation.offset_y = 10 * move.noise_angle[1];
                animation.offset_x = 10 * move.noise_angle[3];
                animation.low_limit = 0;
                show2 = { Layer2 ? render_value(animation) : 0};

                // float radius = radial_filter_radius;   // radius of a radial
                // brightness filter float radial =
                // (radius-distance[x][y])/distance[x][y];
                
                float radius = radial_filter_radius * cRadius;
                radialFilterFalloff = cEdge;
                radialDimmer = radialFilterFactor(radius, distance[x][y], radialFilterFalloff);

                pixel.red = show1 * radialDimmer;
                pixel.green = 0 * radialDimmer;
                pixel.blue = show2 * radialDimmer;

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
    
    void EXPERIMENT_SM1() { 

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

                pixel.red = (show1 + show2) * cRed;
                pixel.green = (show3 + show4) * cGreen;
                pixel.blue = show5 * cBlue;

                pixel = rgb_sanity_check(pixel);
                //leds[xyMap(x, y)] = CRGB(pixel.red, pixel.green, pixel.blue);
                setPixelColorInternal(x, y, pixel);
                //setPixelColorInternal((num_x - 1) - x, y, pixel);
                //setPixelColorInternal((num_x - 1) - x, (num_y - 1) - y, pixel);
                //setPixelColorInternal(x, (num_y - 1) - y, pixel);
            }
        }

    }

    //*******************************************************************************

    void Experiment10() {

        timings.master_speed = 0.01 * cSpeed; 

        timings.ratio[0] = 0.01 + cRatBase/10;
        timings.ratio[1] = 0.011 + cRatBase/10;
        timings.ratio[2] = 0.013 + cRatBase/10;
        timings.ratio[3] = 0.33 + cRatBase * cRatDiff;
        timings.ratio[4] = 0.36 + cRatBase * cRatDiff; 
        timings.ratio[5] = 0.38 + cRatBase * cRatDiff;
        timings.ratio[6] = 0.0003; // master rotation

        timings.offset[0] = 0;
        timings.offset[1] = 100 * cOffBase * cOffDiff;
        timings.offset[2] = 200 * cOffBase * 1.2 * cOffDiff;
        timings.offset[3] = 300 * cOffBase * 1.4 * cOffDiff;
        timings.offset[4] = 400 * cOffBase * 1.6 * cOffDiff;
        timings.offset[5] = 500 * cOffBase * 1.8 * cOffDiff;
        timings.offset[6] = 600 * cOffBase * 2 * cOffDiff;

        calculate_oscillators(timings);

        for (int x = 0; x < num_x; x++) {
            for (int y = 0; y < num_y; y++) {

                float r = 1.5; // scroll speed

                animation.dist =
                    3 + distance[x][y] * cZoom +
                    3 * FL_SIN_F(0.25 * distance[x][y] * cZoom
                    - move.radial[3]);
                animation.angle = 
                    polar_theta[x][y] * cAngle
                    + move.noise_angle[0] 
                    + move.noise_angle[6];
                animation.z = 5 * cZ;
                animation.scale_x = 0.1 * cScale;
                animation.scale_y = 0.1 * cScale;
                animation.offset_z = 10 * move.linear[0];
                animation.offset_y = -5 * r * move.linear[0];
                animation.offset_x = 10;
                animation.low_limit = 0;
                show1 = { Layer1 ? render_value(animation) : 0};

                animation.dist =
                    4 + distance[x][y] * cZoom +
                    4 * FL_SIN_F(0.24 * distance[x][y] * cZoom
                    - move.radial[4]);
                animation.angle = 
                    polar_theta[x][y] * cAngle
                    + move.noise_angle[1] 
                    + move.noise_angle[6];
                animation.z = 5 * cZ;
                animation.scale_x = 0.1 * cScale;
                animation.scale_y = 0.1 * cScale;
                animation.offset_z = 0.1 * move.linear[1];
                animation.offset_y = -5 * r * move.linear[1];
                animation.offset_x = 100;
                animation.low_limit = 0;
                show2 = { Layer2 ? render_value(animation) : 0};

                animation.dist =
                    5 + distance[x][y]
                    + 5 * FL_SIN_F(0.23 * distance[x][y] 
                    - move.radial[5]);
                animation.angle = 
                    polar_theta[x][y] * cAngle 
                    + move.noise_angle[2] 
                    + move.noise_angle[6];
                animation.z = 5 * cZ;
                animation.scale_x = 0.1 * cScale;
                animation.scale_y = 0.1 * cScale;
                animation.offset_z = 0.1 * move.linear[2];
                animation.offset_y = -5 * r * move.linear[2];
                animation.offset_x = 1000;
                animation.low_limit = 0;
                show3 = { Layer3 ? render_value(animation) : 0};

                show4 = colordodge(show1, show2);

                //float rad = FL_SIN_F(PI / 2 + distance[x][y] / 14); // better radial filter?!

                float radius = radial_filter_radius * cRadius;
                radialFilterFalloff = cEdge;
                radialDimmer = radialFilterFactor(radius, distance[x][y], radialFilterFalloff);


                /*
                pixel.red    = show1;
                pixel.green  = show1 * 0.3;
                pixel.blue   = show2-show1;
                */

                CHSV(radialDimmer * ((show1 + show2) + show3), 255, 255);

                pixel = rgb_sanity_check(pixel);

                uint8_t a = getTime() / 100;
                CRGB p = CRGB(CHSV(((a + show1 + show2) + show3), 255, 255));
                rgb pixel;
                pixel.red = p.red * cRed;
                pixel.green = p.green * cGreen;
                pixel.blue = p.blue * cBlue;
                setPixelColorInternal(x, y, pixel);
            }
        }
    }

    //*******************************************************************************

    void Test() {
        
        run_default_oscillators( 0.003);
        //timings.master_speed = 0.003;
        calculate_oscillators(timings);

        for (int x = 0; x < num_x; x++) {
            for (int y = 0; y < num_y; y++) {

                animation.dist = (distance[x][y] * distance[x][y]) * cZoom / 2;
                animation.angle = polar_theta[x][y] * cAngle;

                animation.scale_x = 0.005 * cScale;
                animation.scale_y = 0.005 * cScale;

                animation.offset_y = -10 * move.linear[0];
                animation.offset_x = 0;
                animation.offset_z = 0.1 * move.linear[0];

                animation.z = 0;
                animation.low_limit = 0;
                float show1 = render_value(animation);

                // float linear = 1;//(y+1)/(num_y-1.f);

                pixel.red = show1;
                pixel.green = 0;
                pixel.blue = 40 - show1;

                pixel = rgb_sanity_check(pixel);
                setPixelColorInternal(x, y, pixel);
            }
        }
    }

//*******************************************************************************

};

} // namespace animartrix_detail

// End fast math optimizations
#if FL_ANIMARTRIX_USES_FAST_MATH
FL_OPTIMIZATION_LEVEL_O3_END
FL_FAST_MATH_END
#endif