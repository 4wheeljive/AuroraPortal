#pragma once

#include "bleControl.h"

namespace rainbow {

bool rainbowInstance = false;

void DrawOneFrame( uint8_t startHue8, int8_t yHueDelta8, int8_t xHueDelta8) {
	uint8_t lineStartHue = startHue8;
	for( uint8_t y = 0; y < HEIGHT; y++) {
		lineStartHue += yHueDelta8;
		uint8_t pixelHue = lineStartHue;      
		for( uint8_t x = 0; x < WIDTH; x++) {
			pixelHue += xHueDelta8;
			
			switch (mapping) {
 				case 1:   
					ledNum = loc2indProgByRow[y][x];   
					break;
				case 2:
					ledNum = loc2indSerpByRow[y][x];
					break;
				}
			
			leds[ledNum] = CHSV(pixelHue, 255, 255);
		}  
	}
}

void initRainbow() {
    rainbowInstance = true;
}

void runRainbow() {
	uint32_t ms = millis();
	int32_t yHueDelta32 = ((int32_t)cos16( ms * (27/1) ) * (350 / WIDTH));
	int32_t xHueDelta32 = ((int32_t)cos16( ms * (39/1) ) * (310 / HEIGHT));
	DrawOneFrame( ms / 65536, yHueDelta32 / 32768, xHueDelta32 / 32768);
}

} // namespace rainbow