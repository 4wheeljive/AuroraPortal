#pragma once

#include "bleControl.h"

namespace waves {

	bool wavesInstance = false;

	#define SECONDS_PER_PALETTE 15
	uint16_t hueIncMax = 1500;
	CRGB newcolor = CRGB::Black;
	uint8_t blendFract = 64;

	void initWaves() {
		wavesInstance = true;
		startingPalette();
	}

	void runWaves() {

		if (MODE==0 && rotateWaves) {
			EVERY_N_SECONDS( SECONDS_PER_PALETTE ) {
				//capture the prior target palNum as the current palNum 
				gCurrentPaletteNumber = gTargetPaletteNumber; 
				//then set a new target
				gTargetPaletteNumber = addmod8( gTargetPaletteNumber, 1, gGradientPaletteCount);
				gTargetPalette = gGradientPalettes[ gTargetPaletteNumber ];
				if (debug) {
					Serial.print("Next color palette number: ");
					Serial.println(gTargetPaletteNumber);
				}
			}
		}
		
		EVERY_N_MILLISECONDS(40) {
			nblendPaletteTowardPalette( gCurrentPalette, gTargetPalette, 16); 
		}
		
		switch(MODE){
			case 0: hueIncMax = 1500; break;
			case 1: hueIncMax = 3000; break;
		}

		static uint16_t sPseudotime = 0;
		static uint16_t sLastMillis = 0;
		static uint16_t sHue16 = 0;
	
		uint8_t sat8 = beatsin88( 87, 220, 250); 
		uint8_t brightdepth = beatsin88( 341, 96, 224);
		uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
		uint8_t msmultiplier = beatsin88(147, 23, 60);
	
		uint16_t hue16 = sHue16; 
		uint16_t hueinc16 = beatsin88(113, 1, hueIncMax);
		uint16_t ms = millis();  
		uint16_t deltams = ms - sLastMillis ;
		sLastMillis  = ms;     
		sPseudotime += deltams * msmultiplier*cSpeed;
		sHue16 += deltams * beatsin88( 400, 5,9);  
		uint16_t brightnesstheta16 = sPseudotime;

		for( uint16_t i = 0 ; i < NUM_LEDS; i++ ) {
		
			hue16 += hueinc16;
			uint8_t hue8 = hue16 / 256;

			if (MODE==0) {
				uint16_t h16_128 = hue16 >> 7;
				if( h16_128 & 0x100) {
					hue8 = 255 - (h16_128 >> 1);
				} else {
					hue8 = h16_128 >> 1;
				}
			}

			brightnesstheta16 += brightnessthetainc16;
			uint16_t b16 = sin16( brightnesstheta16 ) + 32768;

			uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
			uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
			bri8 += (255 - brightdepth);
		
			switch (MODE) {
		
				case 0: {
					uint8_t index = hue8;
					index = scale8( index, 240);
					newcolor = ColorFromPalette( gCurrentPalette, index, bri8);
					blendFract = 128;
					break;
				}
				
				case 1: {
					newcolor = CHSV( hue8, sat8, bri8);
					blendFract = 64;
					break;
				}
			}

			ledNum = serpTopDown[i];
				
			nblend( leds[ledNum], newcolor, blendFract);

		}
	FastLED.delay(5);	

	} // runWaves()

} // namespace waves