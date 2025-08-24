#pragma once

#include "bleControl.h"

namespace dots {

	bool dotsInstance = false;

	// Function pointer to access XY function from main.cpp
	uint16_t (*xyFunc)(uint8_t x, uint8_t y);

	void initDots(uint16_t (*xy_func)(uint8_t, uint8_t)) {
		dotsInstance = true;
		xyFunc = xy_func;  
	}

	byte osci[4]; 
	byte pX[4];
	byte pY[4];

	void PixelA(uint8_t x, uint8_t y, byte color) {
		leds[xyFunc(x, y)] = CHSV(color, 255, 255);
	}

	void PixelB(uint8_t x, uint8_t y, byte color) {
		leds[xyFunc(x, y)] = CHSV(color, 255, 255);
	}

	// set the speeds (and by that ratios) of the oscillators
	// JSH add: set separate x and y oscillator arrays 
	void MoveOscillators() {
		osci[0] = osci[0] + 5;
		osci[1] = osci[1] + 2;
		osci[2] = osci[2] + 3;
		osci[3] = osci[3] + 4;
		for(int i = 0; i < 4; i++) { 
			pX[i] = map(sin8(osci[i]),0,255,0,WIDTH-1);
			pY[i] = map(sin8(osci[i]),0,255,0,HEIGHT-1);
		}
	}

	// give it a linear tail downwards
	void VerticalStream(byte scale)  
	{
		for(uint8_t x = 0; x < WIDTH ; x++) {
			for(uint8_t y = 1; y < HEIGHT; y++) { 
				leds[xyFunc(x,y)] += leds[xyFunc(x,y-1)];
				leds[xyFunc(x,y)].nscale8(scale);
			}
		}
		for(uint8_t x = 0; x < WIDTH; x++) 
			leds[xyFunc(x,0)].nscale8(scale);
	}

	/*
	// create a square twister
	// x and y for center, r for radius
	void SpiralStream(int x, int y, int r, byte dim) {
		for (int d = r; d >= 0; d--) { // from the outside to the inside
			for (int i = x - d; i <= x + d; i++) {
				leds[xyFunc(i, y - d)] +=
					leds[xyFunc(i + 1, y - d)]; // lowest row to the right
				leds[xyFunc(i, y - d)].nscale8(dim);
			}
			for (int i = y - d; i <= y + d; i++) {
				leds[xyFunc(x + d, i)] += leds[xyFunc(x + d, i + 1)]; // right colum up
				leds[xyFunc(x + d, i)].nscale8(dim);
			}
			for (int i = x + d; i >= x - d; i--) {
				leds[xyFunc(i, y + d)] +=
					leds[xyFunc(i - 1, y + d)]; // upper row to the left
				leds[xyFunc(i, y + d)].nscale8(dim);
			}
			for (int i = y + d; i >= y - d; i--) {
				leds[xyFunc(x - d, i)] += leds[xyFunc(x - d, i - 1)]; // left colum down
				leds[xyFunc(x - d, i)].nscale8(dim);
			}
		}
	}
 

	// give it a linear tail to the side
	void HorizontalStream(byte scale) {
		for (int x = 1; x < WIDTH; x++) {
			for (int y = 0; y < HEIGHT; y++) {
				leds[xyFunc(x, y)] += leds[xyFunc(x - 1, y)];
				leds[xyFunc(x, y)].nscale8(scale);
			}
		}
		for (int y = 0; y < HEIGHT; y++)
			leds[xyFunc(0, y)].nscale8(scale);
	}
			*/


	void runDots() {

		MoveOscillators();

		PixelA( 
			(pX[2]+pX[0]+pX[1])/3,
			(pY[1]+pY[3]+pY[2])/3,
			osci[1]
		);

		PixelB( 
			(pX[3]+pX[0]+pX[1])/3,
			(pY[1]+pY[0]+pY[2])/3,
			osci[3]
		);
		
		VerticalStream(110 * cTail);
		//HorizontalStream(75);
		FastLED.delay(5);
	}

} // namespace dots