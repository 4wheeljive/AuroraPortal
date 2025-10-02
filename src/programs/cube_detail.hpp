#pragma once

#include "bleControl.h"

namespace cube {

	bool cubeInstance = false;

	// Function pointer to access XY function from main.cpp
	uint16_t (*xyFunc)(uint8_t x, uint8_t y);

	void initCube(uint16_t (*xy_func)(uint8_t, uint8_t)) {
		cubeInstance = true;
		xyFunc = xy_func;  
	}

	// Cube vertices in 3D space
	float vertices[8][3] = {
	{-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
	{-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}
	};
	
	// Cube edges (pairs of vertex indices)
	int edges[12][2] = {
	{0, 1}, {1, 2}, {2, 3}, {3, 0},  // Back face
	{4, 5}, {5, 6}, {6, 7}, {7, 4},  // Front face
	{0, 4}, {1, 5}, {2, 6}, {3, 7}   // Connecting edges
	};
	
	float rotated[8][3];
	float projected[8][2];
	
	float angleX = 0;
	float angleY = 0;
	float angleZ = 0;
		
	// Wu's algorithm for anti-aliased pixel drawing
	void wu_pixel(float x, float y, CRGB color) {
	int x0 = (int)x;
	int y0 = (int)y;
	
	float fx = x - x0;
	float fy = y - y0;
	
	// Calculate weights for the 4 surrounding pixels
	float w00 = (1 - fx) * (1 - fy);
	float w10 = fx * (1 - fy);
	float w01 = (1 - fx) * fy;
	float w11 = fx * fy;
	
	// Draw the 4 pixels with appropriate brightness
	int idx;
	
	idx = xyFunc(x0, y0);
	if (idx >= 0) leds[idx] += CRGB(color.r * w00, color.g * w00, color.b * w00);
	
	idx = xyFunc(x0 + 1, y0);
	if (idx >= 0) leds[idx] += CRGB(color.r * w10, color.g * w10, color.b * w10);
	
	idx = xyFunc(x0, y0 + 1);
	if (idx >= 0) leds[idx] += CRGB(color.r * w01, color.g * w01, color.b * w01);
	
	idx = xyFunc(x0 + 1, y0 + 1);
	if (idx >= 0) leds[idx] += CRGB(color.r * w11, color.g * w11, color.b * w11);
	}
	
	// Draw anti-aliased line using Wu's algorithm
	void wu_line(float x0, float y0, float x1, float y1, CRGB color) {
	bool steep = abs(y1 - y0) > abs(x1 - x0);
	
	if (steep) {
		float tmp;
		tmp = x0; x0 = y0; y0 = tmp;
		tmp = x1; x1 = y1; y1 = tmp;
	}
	
	if (x0 > x1) {
		float tmp;
		tmp = x0; x0 = x1; x1 = tmp;
		tmp = y0; y0 = y1; y1 = tmp;
	}
	
	float dx = x1 - x0;
	float dy = y1 - y0;
	float gradient = (dx == 0) ? 1 : dy / dx;
	
	// Handle first endpoint
	float xend = round(x0);
	float yend = y0 + gradient * (xend - x0);
	float xgap = 1 - fmod(x0 + 0.5, 1);
	int xpxl1 = xend;
	int ypxl1 = (int)yend;
	
	if (steep) {
		wu_pixel(ypxl1, xpxl1, CRGB(color.r * (1 - fmod(yend, 1)) * xgap,
									color.g * (1 - fmod(yend, 1)) * xgap,
									color.b * (1 - fmod(yend, 1)) * xgap));
		wu_pixel(ypxl1 + 1, xpxl1, CRGB(color.r * fmod(yend, 1) * xgap,
										color.g * fmod(yend, 1) * xgap,
										color.b * fmod(yend, 1) * xgap));
	} else {
		wu_pixel(xpxl1, ypxl1, CRGB(color.r * (1 - fmod(yend, 1)) * xgap,
									color.g * (1 - fmod(yend, 1)) * xgap,
									color.b * (1 - fmod(yend, 1)) * xgap));
		wu_pixel(xpxl1, ypxl1 + 1, CRGB(color.r * fmod(yend, 1) * xgap,
										color.g * fmod(yend, 1) * xgap,
										color.b * fmod(yend, 1) * xgap));
	}
	
	float intery = yend + gradient;
	
	// Handle second endpoint
	xend = round(x1);
	yend = y1 + gradient * (xend - x1);
	xgap = fmod(x1 + 0.5, 1);
	int xpxl2 = xend;
	int ypxl2 = (int)yend;
	
	if (steep) {
		wu_pixel(ypxl2, xpxl2, CRGB(color.r * (1 - fmod(yend, 1)) * xgap,
									color.g * (1 - fmod(yend, 1)) * xgap,
									color.b * (1 - fmod(yend, 1)) * xgap));
		wu_pixel(ypxl2 + 1, xpxl2, CRGB(color.r * fmod(yend, 1) * xgap,
										color.g * fmod(yend, 1) * xgap,
										color.b * fmod(yend, 1) * xgap));
	} else {
		wu_pixel(xpxl2, ypxl2, CRGB(color.r * (1 - fmod(yend, 1)) * xgap,
									color.g * (1 - fmod(yend, 1)) * xgap,
									color.b * (1 - fmod(yend, 1)) * xgap));
		wu_pixel(xpxl2, ypxl2 + 1, CRGB(color.r * fmod(yend, 1) * xgap,
										color.g * fmod(yend, 1) * xgap,
										color.b * fmod(yend, 1) * xgap));
	}
	
	// Main loop
	if (steep) {
		for (int x = xpxl1 + 1; x < xpxl2; x++) {
		wu_pixel((int)intery, x, CRGB(color.r * (1 - fmod(intery, 1)),
										color.g * (1 - fmod(intery, 1)),
										color.b * (1 - fmod(intery, 1))));
		wu_pixel((int)intery + 1, x, CRGB(color.r * fmod(intery, 1),
											color.g * fmod(intery, 1),
											color.b * fmod(intery, 1)));
		intery += gradient;
		}
	} else {
		for (int x = xpxl1 + 1; x < xpxl2; x++) {
		wu_pixel(x, (int)intery, CRGB(color.r * (1 - fmod(intery, 1)),
										color.g * (1 - fmod(intery, 1)),
										color.b * (1 - fmod(intery, 1))));
		wu_pixel(x, (int)intery + 1, CRGB(color.r * fmod(intery, 1),
											color.g * fmod(intery, 1),
											color.b * fmod(intery, 1)));
		intery += gradient;
		}
	}
	}
	
	// 3D rotation and projection
	void rotateCube() {
		float cosX = cos(angleX), sinX = sin(angleX);
		float cosY = cos(angleY), sinY = sin(angleY);
		float cosZ = cos(angleZ), sinZ = sin(angleZ);
	
		for (int i = 0; i < 8; i++) {
			float x = vertices[i][0];
			float y = vertices[i][1];
			float z = vertices[i][2];
		
			// Rotate around X axis
			float y1 = y * cosX - z * sinX;
			float z1 = y * sinX + z * cosX;
		
			// Rotate around Y axis
			float x2 = x * cosY + z1 * sinY;
			float z2 = -x * sinY + z1 * cosY;
		
			// Rotate around Z axis
			float x3 = x2 * cosZ - y1 * sinZ;
			float y3 = x2 * sinZ + y1 * cosZ;
		
			rotated[i][0] = x3;
			rotated[i][1] = y3;
			rotated[i][2] = z2;
		
			// Perspective projection
			float scale = MIN_DIMENSION / 4; // 22.0
			projected[i][0] = x3 * scale * cScale + WIDTH / 2.0;
			projected[i][1] = y3 * scale * cScale + HEIGHT / 2.0;
		}
	} // rotateCube()

	void runCube() {

		FastLED.clear();
		
		rotateCube();
		
		// Draw all edges with rainbow colors
		for (int i = 0; i < 12; i++) {
			int v0 = edges[i][0];
			int v1 = edges[i][1];
		
			CRGB color = CHSV(i * 21, 255, 255);
		
			wu_line(projected[v0][0], projected[v0][1],
					projected[v1][0], projected[v1][1],
					color);
		}
		
		//FastLED.show();
		
		// Update rotation angles
		angleX += 0.02;
		angleY += 0.03;
		angleZ += 0.01;
		
		//delay(20);

	} // runCube()

} // namespace cube
