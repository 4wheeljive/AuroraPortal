#pragma once

#include "bleControl.h"

namespace rainbow {

	bool rainbowInstance = false;

	uint16_t (*xyFunc)(uint8_t x, uint8_t y);

	void initRainbow(uint16_t (*xy_func)(uint8_t, uint8_t)) {
		rainbowInstance = true;
		xyFunc = xy_func;
	}

	//Rainbow rainbow(NUM_LEDS);

	void DrawOneFrame( uint8_t startHue8, int8_t yHueDelta8, int8_t xHueDelta8) {
		uint8_t lineStartHue = startHue8;
		for( uint8_t y = 0; y < HEIGHT; y++) {
			lineStartHue += yHueDelta8;
			uint8_t pixelHue = lineStartHue;      
			for( uint8_t x = 0; x < WIDTH; x++) {
				pixelHue += xHueDelta8;
				ledNum = xyFunc(x,y);
				leds[ledNum] = CHSV(pixelHue, 255, 175);
				//rainbow.draw(Fx::DrawContext(millis(), leds));
			}  
		}
	}

	void runRainbow() {
		uint32_t ms = fl::millis();
		float oscRateY = ms * 27 ;
		float oscRateX = ms * 39 ;
		int32_t yHueDelta32 = ((int32_t)cos16( oscRateY ) * 10 );
		int32_t xHueDelta32 = ((int32_t)cos16( oscRateX ) * 10 );
		DrawOneFrame( ms / 65536, yHueDelta32 / 32768, xHueDelta32 / 32768);

		//FastLED.delay(1);
	}

} // namespace rainbow