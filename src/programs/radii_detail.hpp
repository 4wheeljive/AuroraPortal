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

	uint8_t scaledSpeed = 1;
	const float radiusBase = MAX_DIMENSION / 2;

	uint8_t hue = 0;
	//uint8_t hueX = 15;

	struct {
		float angle;
		float distance;
	}
	rMap[WIDTH][HEIGHT];

	void initRadii(uint16_t (*xy_func)(uint8_t, uint8_t)) {
		radiiInstance = true;
		xyFunc = xy_func;  // Store the XY function pointer

		// map polar coordinates 
		for (int8_t x = -center_x; x < center_x + (WIDTH % 2); x++) {
			for (int8_t y = -center_y; y < center_y + (HEIGHT % 2); y++) {
				rMap[x + center_x][y + center_y].angle = 128 * (atan2(y, x) / PI);
				rMap[x + center_x][y + center_y].distance = hypot(x, y) * mapp; //thanks Sutaburosu
			}
		}
	}

    /*float radialFilterFactor( float radius, float distance, float falloff) {
        if (distance >= radius) return 0.0f;
        float factor = 1.0f - (distance / radius);
        return powf(factor, falloff);
    }*/

	void runRadii() {

		FastLED.clear();

		uint8_t speed = cSpeedInt;
		switch(MODE){
			case 0: scaledSpeed = speed 	; break;
			case 1: scaledSpeed = speed 	; break;
			case 2: scaledSpeed = speed 	; break;
			case 3: scaledSpeed = speed 	; break;
			case 4: scaledSpeed = (speed+2)*2 	; break;
		}
		static uint16_t timer;
		timer += scaledSpeed;

		//CHSV cHSVcolor = rgb2hsv_approximate(cColor);
		//hueX = cHSVcolor.h;
		hue = 50;

		for (uint8_t x = 0; x < WIDTH; x++) {
			for (uint8_t y = 0; y < HEIGHT; y++) {
				float angle = rMap[x][y].angle * cAngle;
				float distance = rMap[x][y].distance * cZoom;

				//float radius = radiusBase * cRadius;
                //float radialFilterFalloff = cEdge;
                //float radialDimmer = radialFilterFactor(radius, distance, radialFilterFalloff);

				switch (MODE) {
		
					case 0: // octopus
						leds[xyFunc(x, y)] = CHSV(timer/2 - distance, 255, sin8(sin8((angle*4 - distance) / 4 + timer) + distance - timer*2 + angle*legs));  
						break;
		
					case 1: // flower
						leds[xyFunc(x, y)] = CHSV(timer + distance, 255, sin8(sin8(timer + angle*5 + distance) + timer*4 + sin8(timer*4 - distance) + angle*5));
						break;

					case 2: // lotus
						leds[xyFunc(x, y)] = CHSV(hue,181,sin8(timer-distance+sin8(timer + angle*petals)/5));
						/*
						if (debug) {
							EVERY_N_SECONDS(5) {
								Serial.print("hue: ");
								Serial.println(hue);
								Serial.print("hueX: ");
								Serial.println(hueX);
							}
						}
						*/
						break;
		
					case 3:  // radial waves
						leds[xyFunc(x, y)] = CHSV(timer + distance, 255, sin8(timer*4 + sin8(timer*4 - distance) + angle*3));
						break;

					case 4: // lollipop  			
						static uint8_t scaleX = 4 * cScale;
            			static uint8_t scaleY = 4 * cScale;
						//float radialFilter = distance;
						leds[xyFunc(x, y)] = CHSV( (angle*scaleX) - timer + (distance*scaleY),
													255, 
													constrain(distance*(255/center_y),0,255)
													//255*radialDimmer
												);	
						break;
				}
			}
		}
		
		FastLED.delay(15);
	}

} // namespace radii