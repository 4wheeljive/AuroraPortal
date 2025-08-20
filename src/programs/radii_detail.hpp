#pragma once

#include "bleControl.h"

namespace radii {

	bool radiiInstance = false;

	uint16_t (*xyFunc)(uint8_t x, uint8_t y);

	#define legs 3
	#define petals 5

	const uint8_t center_x = WIDTH / 2;
	const uint8_t center_y = HEIGHT / 2;
	const uint8_t mapp = 255 / MAX_DIMENSION;

	uint8_t hue = 0;
	uint8_t hueX = 15;

	struct {
		uint8_t angle;
		uint8_t radius;
	}
	rMap[WIDTH][HEIGHT];

	void initRadii(uint16_t (*xy_func)(uint8_t, uint8_t)) {
		radiiInstance = true;
		xyFunc = xy_func;  // Store the XY function pointer

		// map polar coordinates 
		for (int8_t x = -center_x; x < center_x + (WIDTH % 2); x++) {
			for (int8_t y = -center_y; y < center_y + (HEIGHT % 2); y++) {
				rMap[x + center_x][y + center_y].angle = 128 * (atan2(y, x) / PI);
				rMap[x + center_x][y + center_y].radius = hypot(x, y) * mapp; //thanks Sutaburosu
			}
		}
	}

	void runRadii() {

		FastLED.clear();

		uint8_t speed = cCustomE;
		static uint16_t timer;
		timer += speed;

		CHSV cHSVcolor = rgb2hsv_approximate(cColor);
		hueX = cHSVcolor.h;
		hue = 50;

		for (uint8_t x = 0; x < WIDTH; x++) {
			for (uint8_t y = 0; y < HEIGHT; y++) {
				byte angle = rMap[x][y].angle * cAngle;
				byte radius = rMap[x][y].radius * cZoom;

				switch (MODE) {
		
					case 0: // octopus
						leds[xyFunc(x, y)] = CHSV(timer / 2 - radius, 255, sin8(sin8((angle * 4 - radius) / 4 + timer) + radius - timer * 2 + angle * legs));  
						break;
		
					case 1: // flower
						leds[xyFunc(x, y)] = CHSV(timer + radius, 255, sin8(sin8(timer + angle * 5 + radius) + timer * 4 + sin8(timer * 4 - radius) + angle * 5));
						break;

					case 2: // lotus
						leds[xyFunc(x, y)] = CHSV(hue,181,sin8(timer-radius+sin8(timer + angle*petals)/5));
						if (debug) {
							EVERY_N_SECONDS(5) {
								Serial.print("hue: ");
								Serial.println(hue);
								Serial.print("hueX: ");
								Serial.println(hueX);
							}
						}
						
						break;
		
					case 3:  // radial waves
						leds[xyFunc(x, y)] = CHSV(timer + radius, 255, sin8(timer * 4 + sin8(timer * 4 - radius) + angle * 3));
						break;
				}
			}
		}
		
		FastLED.delay(15);
	}

} // namespace radii