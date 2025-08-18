#pragma once

#include <Arduino.h>
#include <FastLED.h>

#include "bleControl.h"

namespace bubble {

    bool bubbleInstance = false;

    uint16_t (*xyFunc)(uint8_t x, uint8_t y);

    void initBubble(uint16_t (*xy_func)(uint8_t, uint8_t)) {
        bubbleInstance = true;
        xyFunc = xy_func;
    }

    int8_t zD;
    int8_t zF;
    uint8_t noise3d[WIDTH][HEIGHT];
    uint32_t noise32_x;
    uint32_t noise32_y;
    uint32_t noise32_z;

    uint32_t scale32_x;
    uint32_t scale32_y;
    bool firstRun = true;
    uint8_t noisesmooth;

    uint16_t mov;

    void FillNoise() {
        for (uint8_t i = 0; i < WIDTH; i++) {
            int32_t ioffset = scale32_x * (i - WIDTH / 2);
            for (uint8_t j = 0; j < HEIGHT; j++) {
                int32_t joffset = scale32_y * (j - HEIGHT / 2);
                int8_t data = inoise16(noise32_x + ioffset, noise32_y + joffset, noise32_z) >> 8;
                int8_t olddata = noise3d[i][j];
                int8_t newdata = scale8(olddata, noisesmooth) + scale8(data, 255 - noisesmooth);
                data = newdata;
                noise3d[i][j] = data;     // puts a uint8_t hue value into position [i][j] in the noise3d array
            }
        }
    }

    //*******************************************************************************************

    void MoveFractionalNoiseX(int8_t amplitude = 1, float shift = 0) {
        CRGB ledsbuff[WIDTH];
        for (uint8_t y = 0; y < HEIGHT; y++) {
            int16_t amount = ((int16_t) noise3d[0][y] - 128) * 2 * amplitude + shift * 256;
            int8_t delta = abs(amount) >> 8;
            int8_t fraction = abs(amount) & 255;
            for (uint8_t x = 0; x < WIDTH; x++) {
                if (amount < 0) {
                    zD = x - delta;
                    zF = zD - 1;
                } else {
                    zD = x + delta;
                    zF = zD + 1;
                }
                CRGB PixelA = CRGB::Black;
                if ((zD >= 0) && (zD < WIDTH)) {
                    PixelA = leds[xyFunc(zD, y)];
                } 
                else {
                    PixelA = CHSV(~noise3d[abs(zD)%WIDTH][y]*3,255,255);
                }
                CRGB PixelB = CRGB::Black;
                if ((zF >= 0) && (zF < WIDTH)) {
                    PixelB = leds[xyFunc(zF, y)]; 
                }
                else {
                    PixelB = CHSV(~noise3d[abs(zF)%WIDTH][y]*3,255,255);
                }
                ledsbuff[x] = (PixelA.nscale8(ease8InOutApprox(255 - fraction))) + (PixelB.nscale8(ease8InOutApprox(fraction))); // lerp8by8(PixelA, PixelB, fraction );
            }
            for (uint8_t x = 0; x < WIDTH; x++) {
                leds[xyFunc(x, y)] = ledsbuff[x];
            }
        }
    }

    //*******************************************************************************************

    void MoveFractionalNoiseY(int8_t amplitude = 1, float shift = 0) {
        CRGB ledsbuff[HEIGHT];
        for (uint8_t x = 0; x < WIDTH; x++) {
            int16_t amount = ((int16_t) noise3d[x][0] - 128) * 2 * amplitude + shift * 256;
            int8_t delta = abs(amount) >> 8;
            int8_t fraction = abs(amount) & 255;
            for (uint8_t y = 0; y < HEIGHT; y++) {
                if (amount < 0) {
                    zD = y - delta;
                    zF = zD - 1;
                } else {
                    zD = y + delta;
                    zF = zD + 1;
                }
                CRGB PixelA = CRGB::Black;
                if ((zD >= 0) && (zD < HEIGHT)) PixelA = leds[xyFunc(x, zD)]; else PixelA = CHSV(~noise3d[x][abs(zD)%HEIGHT]*3,255,255); 
                CRGB PixelB = CRGB::Black;
                if ((zF >= 0) && (zF < HEIGHT))PixelB = leds[xyFunc(x, zF)];  else PixelB = CHSV(~noise3d[x][abs(zF)%HEIGHT]*3,255,255);
                ledsbuff[y] = (PixelA.nscale8(ease8InOutApprox(255 - fraction))) + (PixelB.nscale8(ease8InOutApprox(fraction)));
            }
            for (uint8_t y = 0; y < HEIGHT; y++) {
                leds[xyFunc(x, y)] = ledsbuff[y];
            }
        }
    }

    //*******************************************************************************************

    void runBubble() {
        if (firstRun) {
            noise32_x = random16();
            noise32_y = random16();
            noise32_z = random16();
            scale32_x = 160000/WIDTH * cScale;
            scale32_y = 160000/HEIGHT * cScale;
            FillNoise();
            for (byte i = 0; i < WIDTH; i++) {
                for (byte j = 0; j < HEIGHT; j++) {
                    leds[xyFunc(i, j)] = CHSV(~noise3d[i][j]*3,255,BRIGHTNESS);
                }
            }
            firstRun = false;
        }
        mov = 1000 * cSpeed * cSpeed;
        noise32_x += mov;
        noise32_y += mov;
        noise32_z += mov;
        FillNoise();
        
        float noiseMoveFactor = MIN_DIMENSION / 8 * cCustomA;
        MoveFractionalNoiseX(noiseMoveFactor);
        MoveFractionalNoiseY(noiseMoveFactor);
    }


} // namespace bubble