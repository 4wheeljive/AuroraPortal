#pragma once

#include "bleControl.h"
#include "fl/fx/fx.h"
#include "fl/fx/2d/blend.h"
#include "fl/fx/2d/wave.h"

namespace fxWave2d {

	bool fxWave2dInstance = false;

	// Object pointers to access XYMap objects from main.cpp
	XYMap* myXYmapPtr;
	XYMap* xyRectPtr;

	bool firstWave = true;

	bool autoTrigger = true;
	bool easeModeSqrt = false;

	float triggerSpeed = .4f;
	uint8_t blurAmount = 0;
	uint8_t blurPasses = 1;
	float superSample = 1.f;

	//float speedLower = .16f;
	//float dampeningLower = 8.0f;
	bool halfDuplexLower = true;
	uint8_t blurAmountLower = 0;
	uint8_t blurPassesLower = 1;

	//float speedUpper = .24f;
	//float dampeningUpper = 6.0f;
	bool halfDuplexUpper = true;
	uint8_t blurAmountUpper = 12;
	uint8_t blurPassesUpper = 1;

	uint32_t fancySpeed = 796;
	uint8_t fancyIntensity = 32;
	float fancyParticleSpan = .1f;

	//****************************************************************************

	DEFINE_GRADIENT_PALETTE(electricBlueFirePal){
			0,   0,   0,   0,   
			32,  0,   0,   70,  
			128, 20,  57,  255, 
			255, 255, 255, 255 
	};

	DEFINE_GRADIENT_PALETTE(electricGreenFirePal){
			0,   0,   0,   0,  
			8,   128, 64,  64,  
			16,  225, 200, 200, 
			100,  250, 250, 250, 
			255, 255, 255, 255  
	};

	//****************************************************************************

	fl::WaveFx::Args CreateArgsLower() {
			fl::WaveFx::Args out;
			out.factor = fl::SuperSample::SUPER_SAMPLE_4X;
			out.half_duplex = true;
			out.auto_updates = true;  
			out.speed = 0.26f; 
			out.dampening = 7.0f;
			out.crgbMap = fl::make_shared<fl::WaveCrgbGradientMap>(electricBlueFirePal);  
			return out;
	}  

	fl::WaveFx::Args CreateArgsUpper() {
			fl::WaveFx::Args out;
			out.factor = fl::SuperSample::SUPER_SAMPLE_4X; 
			out.half_duplex = true;  
			out.auto_updates = true; 
			out.speed = 0.14f;      
			out.dampening = 6.0f;     
			out.crgbMap = fl::make_shared<fl::WaveCrgbGradientMap>(electricGreenFirePal); 
			return out;
	}

	// For screenTest, these need to use xyRect
	// For LED panel display, these need to use myXYmap
	fl::WaveFx* waveFxLowerPtr = nullptr;
	fl::WaveFx* waveFxUpperPtr = nullptr; 

	// For screenTest, this needs to use myXYmap
	// For LED panel display, this needs to use xyRect
	fl::Blend2d* fxBlendPtr = nullptr;

	//*************************************************************************

	fl::SuperSample getSuperSample() {
			switch (int(superSample)) {
			case 0:		return fl::SuperSample::SUPER_SAMPLE_NONE;
			case 1: 	return fl::SuperSample::SUPER_SAMPLE_2X;
			case 2:		return fl::SuperSample::SUPER_SAMPLE_4X;
			case 3: 	return fl::SuperSample::SUPER_SAMPLE_8X;
			default:	return fl::SuperSample::SUPER_SAMPLE_NONE;
			}
	}

	//*************************************************************************

	void triggerRipple() {

			float perc = .15f;
		
			uint8_t min_x = perc * WIDTH;    
			uint8_t max_x = (1 - perc) * WIDTH; 
			uint8_t min_y = perc * HEIGHT;     
			uint8_t max_y = (1 - perc) * HEIGHT; 
		
			int x = random(min_x, max_x);
			int y = random(min_y, max_y);
			
			waveFxLowerPtr->setf(x, y, 1); 
			waveFxUpperPtr->setf(x, y, 1);

	} // triggerRipple()

	//*************************************************************************

	void applyFancyEffect(uint32_t now, bool button_active) {
		
			uint32_t total = map(fancySpeed, 0, 1000, 1000, 100);
			
			static fl::TimeRamp pointTransition = fl::TimeRamp(0, total, 0);

			if (button_active) {
				pointTransition.trigger(now);
				fancyTrigger = false;
				if (debug) Serial.println("Fancy trigger applied");
			}

			if (!pointTransition.isActive(now)) {
				return;
			}

			int mid_x = WIDTH / 2;
			int mid_y = HEIGHT / 2;
			
			//int MAX_DIMENSION = MAX(WIDTH, HEIGHT);
			int amount = MAX_DIMENSION / 2;
			
			// Calculate the outermost edges for the cross (may be outside of actual matrix??)
			int edgeLeft = mid_x - amount;  
			int edgeRight = mid_x + amount;  
			int edgeBottom = mid_y - amount;
			int edgeTop = mid_y + amount;
			
			int curr_alpha = pointTransition.update8(now);
			
			// Map the animation progress to the four points of the expanding cross
			// As curr_alpha increases from 0 to 255, these points move from center to edges
			int left_x = map(curr_alpha, 0, 255, mid_x, edgeLeft);  
			int right_x = map(curr_alpha, 0, 255, mid_x, edgeRight);  
			int down_y = map(curr_alpha, 0, 255, mid_y, edgeBottom);
			int up_y = map(curr_alpha, 0, 255, mid_y, edgeTop);

			// Convert the 0-255 alpha to 0.0-1.0 range
			float curr_alpha_f = curr_alpha / 255.0f;
		
			// Calculate the wave height value - starts high and decreases as animation progresses
			// This makes the waves stronger at the beginning of the animation
			float valuef = (1.0f - curr_alpha_f) * fancyIntensity/ 255.0f;

			// Thickness of the cross lines
			int span = fl::fl_max(fancyParticleSpan * MAX_DIMENSION, 1) ;
	
			// Add wave energy along the four expanding lines of the cross
			// Each line is a horizontal or vertical span of pixels

			// Left-moving horizontal line
			for (int x = left_x - span; x < left_x + span; x++) {
				waveFxLowerPtr->addf(x, mid_y, valuef); 
				waveFxUpperPtr->addf(x, mid_y, valuef); 
			}

			// Right-moving horizontal line
			for (int x = right_x - span; x < right_x + span; x++) {
				waveFxLowerPtr->addf(x, mid_y, valuef);
				waveFxUpperPtr->addf(x, mid_y, valuef);
			}

			// Downward-moving vertical line
			for (int y = down_y - span; y < down_y + span; y++) {
				waveFxLowerPtr->addf(mid_x, y, valuef);
				waveFxUpperPtr->addf(mid_x, y, valuef);
			}

			// Upward-moving vertical line
			for (int y = up_y - span; y < up_y + span; y++) {
				waveFxLowerPtr->addf(mid_x, y, valuef);
				waveFxUpperPtr->addf(mid_x, y, valuef);
			}
	} // applyFancyEffect()

	//*************************************************************************

	void waveConfig() {
			
			fl::U8EasingFunction easeMode = easeModeSqrt
				? fl::U8EasingFunction::WAVE_U8_MODE_SQRT
				: fl::U8EasingFunction::WAVE_U8_MODE_LINEAR;
			
			waveFxLowerPtr->setSpeed(cSpeedLower);             
			waveFxLowerPtr->setDampening(cDampLower);      
			waveFxLowerPtr->setHalfDuplex(halfDuplexLower);    
			waveFxLowerPtr->setSuperSample(getSuperSample());  
			waveFxLowerPtr->setEasingMode(easeMode);           
			
			waveFxUpperPtr->setSpeed(cSpeedUpper) ;             
			waveFxUpperPtr->setDampening(cDampUpper);     
			waveFxUpperPtr->setHalfDuplex(halfDuplexUpper);   
			waveFxUpperPtr->setSuperSample(getSuperSample()); 
			waveFxUpperPtr->setEasingMode(easeMode);      
		
			fxBlendPtr->setGlobalBlurAmount(blurAmount * cBlurGlobFact);      
			fxBlendPtr->setGlobalBlurPasses(blurPasses);     

			// Create parameter structures for each wave layer's blur settings
			fl::Blend2dParams lower_params;
			lower_params.blur_amount = blurAmountLower;
			lower_params.blur_passes = blurPassesLower;

			fl::Blend2dParams upper_params;
			upper_params.blur_amount = blurAmountUpper;
			upper_params.blur_passes = blurPassesUpper; 

			fxBlendPtr->setParams(*waveFxLowerPtr, lower_params);
			fxBlendPtr->setParams(*waveFxUpperPtr, upper_params);

	} // waveConfig()

	//*************************************************************************

	void processAutoTrigger(uint32_t now) {
		
		static uint32_t nextTrigger = 0;
		
		uint32_t trigger_delta = nextTrigger - now;
		
		if (trigger_delta > 10000) {
				trigger_delta = 0;
		}
		
		if (autoTrigger) {
			if (now >= nextTrigger) {
				triggerRipple();

				float speed = 1.0f - triggerSpeed * cSpeed;
				uint32_t min_rand = 350 * speed; 
				uint32_t max_rand = 2500 * speed; 

				uint32_t min = fl::fl_min(min_rand, max_rand);
				uint32_t max = fl::fl_max(min_rand, max_rand);
				
				if (min == max) {
					max += 1;
				}
				
				nextTrigger = now + random(min, max);
			}
		}
	} // processAutoTrigger()

	//*************************************************************************

	void initFxWave2d(XYMap& myXYmap, XYMap& xyRect) {
		fxWave2dInstance = true;
		
		// Store XYMap references
		myXYmapPtr = &myXYmap;
		xyRectPtr = &xyRect;
		
		// Create the fx objects now that we have the XYMaps
		waveFxLowerPtr = new fl::WaveFx(myXYmap, CreateArgsLower());
		waveFxUpperPtr = new fl::WaveFx(myXYmap, CreateArgsUpper());
		fxBlendPtr = new fl::Blend2d(xyRect);
		
		// Reset setup flag when initializing
		firstWave = true;
	}

	//*************************************************************************

	void runFxWave2d() {

		if (firstWave) {
			fxBlendPtr->add(*waveFxLowerPtr);
			fxBlendPtr->add(*waveFxUpperPtr);
			firstWave = false;
		}
		
		uint32_t now = fl::millis();
		waveConfig();
		EVERY_N_MILLISECONDS_RANDOM (4000,9000) { fancyTrigger = true; }
		applyFancyEffect(now, fancyTrigger);
		processAutoTrigger(now);
		//Fx::DrawContext ctx(now, leds);
		fl::Fx::DrawContext ctx(now, leds);
		fxBlendPtr->draw(ctx);
	}

} // namespace fxWave2d