#pragma once

#include "bleControl.h"

namespace radii {

bool radiiInstance = false;

// Function pointer to access XY function from main.cpp
uint16_t (*xyFunc)(uint8_t x, uint8_t y);

// Radii global variables - moved from main.cpp
#define legs 3
bool setupm = 1;
const uint8_t C_X = WIDTH / 2;
const uint8_t C_Y = HEIGHT / 2;
const uint8_t mapp = 255 / MAX_DIMENSION;
struct {
	uint8_t angle;
	uint8_t radius;
}
rMap[WIDTH][HEIGHT];

void initRadii(uint16_t (*xy_func)(uint8_t, uint8_t)) {
    radiiInstance = true;
    xyFunc = xy_func;  // Store the XY function pointer
    // Reset setup flag when initializing
    setupm = 1;
}

void runRadii() {
    #define petals 5

	FastLED.clear();

	if (setupm) {
		setupm = 0;
		for (int8_t x = -C_X; x < C_X + (WIDTH % 2); x++) {
			for (int8_t y = -C_Y; y < C_Y + (HEIGHT % 2); y++) {
				rMap[x + C_X][y + C_Y].angle = 128 * (atan2(y, x) / PI);
				rMap[x + C_X][y + C_Y].radius = hypot(x, y) * mapp; //thanks Sutaburosu
			}
		}
	}

	static byte speed = 4;
	static uint16_t t;
	t += speed;

	for (uint8_t x = 0; x < WIDTH; x++) {
		for (uint8_t y = 0; y < HEIGHT; y++) {
			byte angle = rMap[x][y].angle;
			byte radius = rMap[x][y].radius;

			switch (MODE) {
	
				case 0: // octopus
					leds[xyFunc(x, y)] = CHSV(t / 2 - radius, 255, sin8(sin8((angle * 4 - radius) / 4 + t) + radius - t * 2 + angle * legs));  
					break;
	
				case 1: // flower
					leds[xyFunc(x, y)] = CHSV(t + radius, 255, sin8(sin8(t + angle * 5 + radius) + t * 4 + sin8(t * 4 - radius) + angle * 5));
					break;

				case 2: // lotus
					leds[xyFunc(x, y)] = CHSV(248,181,sin8(t-radius+sin8(t + angle*petals)/5));
					break;
	
				case 3:  // radial waves
					leds[xyFunc(x, y)] = CHSV(t + radius, 255, sin8(t * 4 + sin8(t * 4 - radius) + angle * 3));
					break;

			}
		}
	}
	
	FastLED.delay(15);
}

} // namespace radii