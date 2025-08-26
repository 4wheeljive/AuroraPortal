#pragma once

#include "bleControl.h"

namespace test {

	bool testInstance = false;

	uint16_t (*xyFunc)(uint8_t x, uint8_t y);
	
	void initTest(uint16_t (*xy_func)(uint8_t, uint8_t)) {
		testInstance = true;
		xyFunc = xy_func;  // Store the XY function pointer
	}

    //***************************************************************************



  /*
    //  Plasma Waves **********************

            static const uint8_t exp_gamma[256] = {
            0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   1,
            1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
            1,   2,   2,   2,   2,   2,   2,   2,   2,   2,   3,   3,   3,   3,   3,
            4,   4,   4,   4,   4,   5,   5,   5,   5,   5,   6,   6,   6,   7,   7,
            7,   7,   8,   8,   8,   9,   9,   9,   10,  10,  10,  11,  11,  12,  12,
            12,  13,  13,  14,  14,  14,  15,  15,  16,  16,  17,  17,  18,  18,  19,
            19,  20,  20,  21,  21,  22,  23,  23,  24,  24,  25,  26,  26,  27,  28,
            28,  29,  30,  30,  31,  32,  32,  33,  34,  35,  35,  36,  37,  38,  39,
            39,  40,  41,  42,  43,  44,  44,  45,  46,  47,  48,  49,  50,  51,  52,
            53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,
            68,  70,  71,  72,  73,  74,  75,  77,  78,  79,  80,  82,  83,  84,  85,
            87,  89,  91,  92,  93,  95,  96,  98,  99,  100, 101, 102, 105, 106, 108,
            109, 111, 112, 114, 115, 117, 118, 120, 121, 123, 125, 126, 128, 130, 131,
            133, 135, 136, 138, 140, 142, 143, 145, 147, 149, 151, 152, 154, 156, 158,
            160, 162, 164, 165, 167, 169, 171, 173, 175, 177, 179, 181, 183, 185, 187,
            190, 192, 194, 196, 198, 200, 202, 204, 207, 209, 211, 213, 216, 218, 220,
            222, 225, 227, 229, 232, 234, 236, 239, 241, 244, 246, 249, 251, 253, 254,
            255};

        int64_t frameCount = 0;

        void runTest() {
        EVERY_N_MILLISECONDS(1000 / 60) {
            frameCount++;
        }

        uint8_t t1 = cos8((42 * frameCount) / 30);
        uint8_t t2 = cos8((35 * frameCount) / 30);
        uint8_t t3 = cos8((38 * frameCount) / 30);

        for (uint16_t y = 0; y < HEIGHT; y++) {
            for (uint16_t x = 0; x < WIDTH; x++) {
            //uint16_t pixelIndex = gridIndexHorizontal(x, y);

            // Calculate 3 seperate plasma waves, one for each color channel
            uint8_t r = cos8((x << 3) + (t1 >> 1) + cos8(t2 + (y << 3)));
            uint8_t g = cos8((y << 3) + t1 + cos8((t3 >> 2) + (x << 3)));
            uint8_t b = cos8((y << 3) + t2 + cos8(t1 + x + (g >> 2)));

            // uncomment the following to enable gamma correction
            // r = pgm_read_byte_near(exp_gamma + r);
            r = exp_gamma[r];
            g = exp_gamma[g];
            b = exp_gamma[b];

            // g = pgm_read_byte_near(exp_gamma + g);
            // b = pgm_read_byte_near(exp_gamma + b);

            leds[xyFunc(x, y)] = CRGB(r, g, b);
            }
        }
        }

*/













    /*
    // Weave **************************
    //https://editor.soulmatelights.com/gallery/255-twist
    byte hue = 0;

    void patt1(uint8_t i, uint8_t j, uint8_t color1, uint8_t color2) {
    //  leds[XY(i, j)] = CHSV(0, 255, 0);
    leds[xyFunc(i + 1, j)] = CHSV(color1, 255, BRIGHTNESS);
    leds[xyFunc(i + 1, j + 1)] = CHSV(color1, 255, BRIGHTNESS);
    leds[xyFunc(i, j + 1)] = CHSV(color2, 255, BRIGHTNESS);
    }

    void patt2(uint8_t i, uint8_t j, uint8_t color1, uint8_t color2) {
    //  leds[XY(i, j)] = CHSV(0, 255, 0);
    leds[xyFunc(i + 1, j)] = CHSV(color1, 255, BRIGHTNESS);
    leds[xyFunc(i + 1, j + 1)] = CHSV(color2, 255, BRIGHTNESS);
    leds[xyFunc(i, j + 1)] = CHSV(color2, 255, BRIGHTNESS);
    }

    void runTest() {
  
        EVERY_N_MILLISECONDS(50) { hue++; }
        
        for (byte i = 0; i < WIDTH; i += 4) {
                for (byte j = 0; j < HEIGHT; j += 4) {
                    patt1(i, j, 64 + j + hue, i + hue);
                    patt1(i + 2, j + 2, 64 + j + hue, i + hue);
                    patt2(i, j + 2, 64 + j + hue, i + hue);
                    patt2(i + 2, j, 64 + j + hue, i + hue);
                }
            }
        }

    // Weave **************************
    */

    // Python ************************
    
    extern const TProgmemRGBPalette16 CopperFireColors_p FL_PROGMEM = {CRGB::Black, 0x001a00, 0x003300, 0x004d00, 0x006600, CRGB::Green, 0x239909, 0x45b313, 0x68cc1c, 0x8ae626, CRGB::GreenYellow, 0x94f530, 0x7ceb30, 0x63e131, 0x4bd731, CRGB::LimeGreen};     //* Green

	void runTest() {

        static uint8_t deltaValue;
        float t = ( millis() + deltaValue) * cSpeed / 50;

        EaseType ease_sat = getEaseType(cEaseSat);
        EaseType ease_lum = getEaseType(cEaseLum);

        for (uint8_t y = 0; y < HEIGHT; y++) {
            for (uint8_t x  = 0; x < WIDTH; x++) {

            leds[xyFunc(x, y)] = ColorFromPalette(CopperFireColors_p, 
                ((sin8((x * 16) + sin8(y * 5 - t * 5.)) + cos8(y / 2 * 10)) + 1) + t)
                .colorBoost(ease_sat, ease_lum);
            }
        }

        if (deltaValue > HEIGHT * 2) {
            deltaValue = 0;
        }

        deltaValue++;
	}
    

    // COOL *******************
    /*
    static const uint8_t exp_gamma[256] = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   1,
    1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
    1,   2,   2,   2,   2,   2,   2,   2,   2,   2,   3,   3,   3,   3,   3,
    4,   4,   4,   4,   4,   5,   5,   5,   5,   5,   6,   6,   6,   7,   7,
    7,   7,   8,   8,   8,   9,   9,   9,   10,  10,  10,  11,  11,  12,  12,
    12,  13,  13,  14,  14,  14,  15,  15,  16,  16,  17,  17,  18,  18,  19,
    19,  20,  20,  21,  21,  22,  23,  23,  24,  24,  25,  26,  26,  27,  28,
    28,  29,  30,  30,  31,  32,  32,  33,  34,  35,  35,  36,  37,  38,  39,
    39,  40,  41,  42,  43,  44,  44,  45,  46,  47,  48,  49,  50,  51,  52,
    53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,
    68,  70,  71,  72,  73,  74,  75,  77,  78,  79,  80,  82,  83,  84,  85,
    87,  89,  91,  92,  93,  95,  96,  98,  99,  100, 101, 102, 105, 106, 108,
    109, 111, 112, 114, 115, 117, 118, 120, 121, 123, 125, 126, 128, 130, 131,
    133, 135, 136, 138, 140, 142, 143, 145, 147, 149, 151, 152, 154, 156, 158,
    160, 162, 164, 165, 167, 169, 171, 173, 175, 177, 179, 181, 183, 185, 187,
    190, 192, 194, 196, 198, 200, 202, 204, 207, 209, 211, 213, 216, 218, 220,
    222, 225, 227, 229, 232, 234, 236, 239, 241, 244, 246, 249, 251, 253, 254,
    255};

    void runTest() {

        int a = millis()/32;
        for (int x = 0; x < WIDTH; x++) {
            for (int y = 0; y < HEIGHT; y++) {
                int index = xyFunc(x, y);
                leds[index].b=exp_gamma [sin8((x-8)*cos8((y+20)*4)/4+a)];
                leds[index].g=exp_gamma [(sin8(x*16+a/3)+cos8(y*8+a/2))/2];
                leds[index].r=exp_gamma [sin8(cos8(x*8+a/3)+sin8(y*8+a/4)+a)];
            }
        }
    }
    */

} // namespace test