#pragma once

#include "bleControl.h"
#include "audioInput.h"
#include "fl/fft.h"
#include "fl/xymap.h"
#include "fl/math.h"
#include "fl/math_macros.h"

using fl::i16;

namespace audioReactive {

	using namespace audioInput;

	bool audioReactiveInstance = false;

    uint16_t (*xyFunc)(uint8_t x, uint8_t y);
	uint8_t hue = 0;

    void initAudioReactive(uint16_t (*xy_func)(uint8_t, uint8_t)) {
        audioReactiveInstance = true;
        xyFunc = xy_func;
        
        // Initialize audio input system
        audioInput::initAudio();
	}
	
	
	UIDropdown visualMode("Visualization Mode", 
		{"Spectrum Bars", "Radial Spectrum", "Waveform", "VU Meter", "Matrix Rain", "Fire Effect", "Plasma Wave"});
	//uint8_t cFadeSpeed = 20; 
	UIDropdown colorPalette("Color Palette", 
		{"Rainbow", "Heat", "Ocean", "Forest", "Party", "Lava", "Cloud"});
	
	// Get current color palette
	CRGBPalette16 getCurrentPalette() {
		
		switch(colorPalette.as_int()) {
			case 0: return CRGBPalette16(RainbowColors_p);
			case 1: return CRGBPalette16(HeatColors_p);
			case 2: return CRGBPalette16(OceanColors_p);
			case 3: return CRGBPalette16(ForestColors_p);
			case 4: return CRGBPalette16(PartyColors_p);
			case 5: return CRGBPalette16(LavaColors_p);
			case 6: return CRGBPalette16(CloudColors_p);
			default: return CRGBPalette16(RainbowColors_p);
		}
	}

	// Clear display
	void clearDisplay() {
		if (cFadeSpeed == 0) {
			fill_solid(leds, NUM_LEDS, CRGB::Black);
		} else {
			fadeToBlackBy(leds, NUM_LEDS, cFadeSpeed);
		}
	}


    //===============================================================================================

	// Visualization: Spectrum Bars
	void drawSpectrumBars(FFTBins* fft, float /* peak */) {
		clearDisplay();
		CRGBPalette16 palette = getCurrentPalette();
		
		int barWidth = WIDTH / NUM_BANDS;
		
		for (size_t band = 0; band < NUM_BANDS && band < fft->bins_db.size(); band++) {
			float magnitude = fft->bins_db[band];
			
			// Apply noise floor
			magnitude = magnitude / 100.0f;  // Normalize from dB
			magnitude = MAX(0.0f, magnitude - cNoiseFloor);
			
			// Smooth the FFT
			fftSmooth[band] = fftSmooth[band] * 0.8f + magnitude * 0.2f;
			magnitude = fftSmooth[band];
			
			// Apply gain
			magnitude *= cAudioGain * autoGainValue;
			magnitude = fl::clamp(magnitude, 0.0f, 1.0f);
			
			int barHeight = magnitude * HEIGHT;
			int xStart = band * barWidth;
			
			for (int x = 0; x < barWidth - 1; x++) {
				for (int y = 0; y < barHeight; y++) {
					uint8_t colorIndex = fl::map_range<float, uint8_t>(
						float(y) / HEIGHT, 0, 1, 0, 255
					);
					CRGB color = ColorFromPalette(palette, colorIndex + hue);
					
					int ledIndex = xyFunc(xStart + x, y);
					if (ledIndex >= 0 && ledIndex < NUM_LEDS) {
						leds[ledIndex] = color;
					}
					
					if (cMirrorMode) {
						int mirrorIndex = xyFunc(WIDTH - 1 - (xStart + x), y);
						if (mirrorIndex >= 0 && mirrorIndex < NUM_LEDS) {
							leds[mirrorIndex] = color;
						}
					}
				}
			}
		}
	}

	// Visualization: Radial Spectrum
	void drawRadialSpectrum(FFTBins* fft, float /* peak */) {
		clearDisplay();
		CRGBPalette16 palette = getCurrentPalette();
		
		int centerX = WIDTH / 2;
		int centerY = HEIGHT / 2;
		
		for (size_t angle = 0; angle < 360; angle += 6) {  // Reduced resolution
			size_t band = (angle / 6) % NUM_BANDS;
			if (band >= fft->bins_db.size()) continue;
			
			float magnitude = fft->bins_db[band] / 100.0f;
			magnitude = MAX(0.0f, magnitude - cNoiseFloor);
			magnitude *= cAudioGain * autoGainValue;
			magnitude = fl::clamp(magnitude, 0.0f, 1.0f);
			
			int radius = magnitude * (MIN(WIDTH, HEIGHT) / 2);
			
			for (int r = 0; r < radius; r++) {
				int x = centerX + (r * cosf(angle * PI / 180.0f));
				int y = centerY + (r * sinf(angle * PI / 180.0f));
				
				if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
					uint8_t colorIndex = fl::map_range<int, uint8_t>(r, 0, radius, 255, 0);
					int ledIndex = xyFunc(x, y);
					if (ledIndex >= 0 && ledIndex < NUM_LEDS) {
						leds[ledIndex] = ColorFromPalette(palette, colorIndex + hue);
					}
				}
			}
		}
	}

	// Visualization: Logarithmic Waveform (prevents saturation)
	void drawWaveform(const Slice<const int16_t>& pcm, float /* peak */) {
		clearDisplay();
		CRGBPalette16 palette = getCurrentPalette();
		
		int samplesPerPixel = pcm.size() / WIDTH;
		int centerY = HEIGHT / 2;
		
		for (size_t x = 0; x < WIDTH; x++) {
			size_t sampleIndex = x * samplesPerPixel;
			if (sampleIndex >= pcm.size()) break;
			
			// Get the raw sample value
			float sample = float(pcm[sampleIndex]) / 32768.0f;  // Normalize to -1.0 to 1.0
			
			// Apply logarithmic scaling to prevent saturation
			float absSample = fabsf(sample);
			float logAmplitude = 0.0f;
			
			if (absSample > 0.001f) {  // Avoid log(0)
				// Logarithmic compression: log10(1 + gain * sample)
				float scaledSample = absSample * cAudioGain * autoGainValue;
				logAmplitude = log10f(1.0f + scaledSample * 9.0f) / log10f(10.0f);  // Normalize to 0-1
			}
			
			// Apply smooth sensitivity curve
			logAmplitude = powf(logAmplitude, 0.7f);  // Gamma correction for better visual response
			
			// Calculate amplitude in pixels
			int amplitude = int(logAmplitude * (HEIGHT / 2));
			amplitude = fl::clamp(amplitude, 0, HEIGHT / 2);
			
			// Preserve the sign for proper waveform display
			if (sample < 0) amplitude = -amplitude;
			
			// Color mapping based on amplitude intensity
			uint8_t colorIndex = fl::map_range<int, uint8_t>(abs(amplitude), 0, HEIGHT/2, 40, 255);
			CRGB color = ColorFromPalette(palette, colorIndex + hue);
			
			// Apply brightness scaling for low amplitudes
			if (abs(amplitude) < HEIGHT / 4) {
				color.fadeToBlackBy(128 - (abs(amplitude) * 512 / HEIGHT));
			}
			
			// Draw vertical line from center
			if (amplitude == 0) {
				// Draw center point for zero amplitude
				int ledIndex = xyFunc(x, centerY);
				if (ledIndex >= 0 && ledIndex < NUM_LEDS) {
					leds[ledIndex] = color.fadeToBlackBy(200);
				}
			} else {
				// Draw line from center to amplitude
				int startY = (amplitude > 0) ? centerY : centerY + amplitude;
				int endY = (amplitude > 0) ? centerY + amplitude : centerY;
				
				for (int y = startY; y <= endY; y++) {
					if (y >= 0 && y < HEIGHT) {
						int ledIndex = xyFunc(x, y);
						if (ledIndex >= 0 && ledIndex < NUM_LEDS) {
							// Fade edges for smoother appearance
							CRGB pixelColor = color;
							if (y == startY || y == endY) {
								pixelColor.fadeToBlackBy(100);
							}
							leds[ledIndex] = pixelColor;
						}
					}
				}
			}
		}
	}

	// Visualization: VU Meter
	void drawVUMeter(float rms, float peak) {
		clearDisplay();
		CRGBPalette16 palette = getCurrentPalette();
		
		// RMS level bar
		int rmsWidth = rms * WIDTH * cAudioGain * autoGainValue;
		rmsWidth = MIN(rmsWidth, WIDTH);
		
		for (int x = 0; x < rmsWidth; x++) {
			for (int y = HEIGHT/3; y < 2*HEIGHT/3; y++) {
				uint8_t colorIndex = fl::map_range<int, uint8_t>(x, 0, WIDTH, 0, 255);
				int ledIndex = xyFunc(x, y);
				if (ledIndex >= 0 && ledIndex < NUM_LEDS) {
					leds[ledIndex] = ColorFromPalette(palette, colorIndex);
				}
			}
		}
		
		// Peak indicator
		int peakX = peak * WIDTH * cAudioGain * autoGainValue;
		peakX = MIN(peakX, WIDTH - 1);
		
		for (int y = HEIGHT/4; y < 3*HEIGHT/4; y++) {
			int ledIndex = xyFunc(peakX, y);
			if (ledIndex >= 0 && ledIndex < NUM_LEDS) {
				leds[ledIndex] = CRGB::White;
			}
		}
		
		// Beat indicator
		if (isBeat && cBeatFlash) {
			for (int x = 0; x < WIDTH; x++) {
				int ledIndex1 = xyFunc(x, 0);
				int ledIndex2 = xyFunc(x, HEIGHT - 1);
				if (ledIndex1 >= 0 && ledIndex1 < NUM_LEDS) leds[ledIndex1] = CRGB::White;
				if (ledIndex2 >= 0 && ledIndex2 < NUM_LEDS) leds[ledIndex2] = CRGB::White;
			}
		}
	}

	// Visualization: Matrix Rain
	void drawMatrixRain(float peak) {
		// Shift everything down
		for (int x = 0; x < WIDTH; x++) {
			for (int y = HEIGHT - 1; y > 0; y--) {
				int currentIndex = xyFunc(x, y);
				int aboveIndex = xyFunc(x, y - 1);
				if (currentIndex >= 0 && currentIndex < NUM_LEDS && 
					aboveIndex >= 0 && aboveIndex < NUM_LEDS) {
					leds[currentIndex] = leds[aboveIndex];
					leds[currentIndex].fadeToBlackBy(40);
				}
			}
		}
		
		// Add new drops based on audio
		int numDrops = peak * WIDTH * cAudioGain * autoGainValue;
		for (int i = 0; i < numDrops; i++) {
			int x = random(WIDTH);
			int ledIndex = xyFunc(x, 0);
			if (ledIndex >= 0 && ledIndex < NUM_LEDS) {
				leds[ledIndex] = CHSV(96, 255, 255);  // Green
			}
		}
	}

	// Visualization: Fire Effect (simplified for WebAssembly)
	void drawFireEffect(float peak) {
		// Simple fire effect without buffer
		clearDisplay();
		
		// Add heat at bottom based on audio
		int heat = 100 + (peak * 155 * cAudioGain * autoGainValue);
		heat = MIN(heat, 255);
		
		for (int x = 0; x < WIDTH; x++) {
			for (int y = 0; y < HEIGHT; y++) {
				// Simple gradient from bottom to top
				int heatLevel = heat * (HEIGHT - y) / HEIGHT;
				heatLevel = heatLevel * random(80, 120) / 100;  // Add randomness
				heatLevel = MIN(heatLevel, 255);
				
				int ledIndex = xyFunc(x, y);
				if (ledIndex >= 0 && ledIndex < NUM_LEDS) {
					leds[ledIndex] = HeatColor(heatLevel);
				}
			}
		}
	}

	// Visualization: Plasma Wave
	void drawPlasmaWave(float peak) {
		static float time = 0;
		time += 0.05f + (peak * 0.2f);
		
		CRGBPalette16 palette = getCurrentPalette();
		
		for (int x = 0; x < WIDTH; x++) {
			for (int y = 0; y < HEIGHT; y++) {
				float value = sinf(x * 0.1f + time) + 
							sinf(y * 0.1f - time) +
							sinf((x + y) * 0.1f + time) +
							sinf(sqrtf(x * x + y * y) * 0.1f - time);
				
				value = (value + 4) / 8;  // Normalize to 0-1
				value *= cAudioGain * autoGainValue;
				
				uint8_t colorIndex = value * 255;
				int ledIndex = xyFunc(x, y);
				if (ledIndex >= 0 && ledIndex < NUM_LEDS) {
					leds[ledIndex] = ColorFromPalette(palette, colorIndex + hue);
				}
			}
		}
	}


	//==================================================================================


	void runAudioReactive() {

		// Update color animation
		hue += 1;
		
		sampleAudio();

		// Apply beat flash
		if (isBeat && cBeatFlash) {
			for (int i = 0; i < NUM_LEDS; i++) {
				leds[i].fadeLightBy(-50);  // Make brighter
			}
		}

		switch (MODE) {

			case 0:  // Spectrum Bars
				drawSpectrumBars(&fftBins, peakLevel);
				break;
				
			case 1:  // Radial Spectrum
				drawRadialSpectrum(&fftBins, peakLevel);
				break;
				
			case 2:  // Waveform
				drawWaveform(currentSample.pcm(), peakLevel);
				break;
				
			case 3:  // VU Meter
				drawVUMeter(currentRms, peakLevel);
				break;
				
			case 4:  // Matrix Rain
				drawMatrixRain(peakLevel);
				break;
				
			case 5:  // Fire Effect
				drawFireEffect(peakLevel);
				break;
				
			case 6:  // Plasma Wave
				drawPlasmaWave(peakLevel);
				break;
		}
	} // runAudioReactive()
		
}  // namespace audioReactive
