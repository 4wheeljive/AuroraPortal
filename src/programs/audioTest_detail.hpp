#pragma once
#include "audioProcessing.h"

namespace audioTest {

	using namespace myAudio;

	bool audioTestInstance = false;

    uint16_t (*xyFunc)(uint8_t x, uint8_t y);

	// Set to true to run audio diagnostics instead of visualizations
	// Use this to calibrate and verify audio input is working correctly
	constexpr bool DIAGNOSTIC_MODE = false;

	uint8_t hue = 0;
	uint8_t visualizationMode = 5;  // 0=spectrum, 1=VU meter, 2=beat pulse, 3=bass ripple, 4=flBeatDetection, 5=radial spectrum, 6=waveform
	uint8_t fadeSpeed = 20;
	uint8_t currentPaletteNum = 1;

	bool needsFftForMode() {
		return (visualizationMode == 0) || (visualizationMode == 5);
	}

    void initAudioTest(uint16_t (*xy_func)(uint8_t, uint8_t)) {
        audioTestInstance = true;
        xyFunc = xy_func;
        myAudio::initAudioInput();
		myAudio::initAudioProcessing();
		startingPalette();
	}

	// Get current color palette
	/*CRGBPalette16 getCurrentPalette() {
		switch(currentPaletteNum) {
			case 0: return CRGBPalette16(RainbowColors_p);
			case 1: return CRGBPalette16(HeatColors_p);
			case 2: return CRGBPalette16(OceanColors_p);
			case 3: return CRGBPalette16(ForestColors_p);
			case 4: return CRGBPalette16(PartyColors_p);
			case 5: return CRGBPalette16(LavaColors_p);
			case 6: return CRGBPalette16(CloudColors_p);
			default: return CRGBPalette16(RainbowColors_p);
		}
	}*/

	void clearDisplay() {
		if (fadeSpeed == 0) {
			fill_solid(leds, NUM_LEDS, CRGB::Black);
		} else {
			fadeToBlackBy(leds, NUM_LEDS, fadeSpeed);
		}
	}

	//===============================================================================================
	// VISUALIZATION MODE 0: Spectrum Analyzer
	// Shows 16 FFT frequency bins as vertical bars across the matrix
	//===============================================================================================
	
	void drawSpectrum(const myAudio::AudioFrame& frame) {
		//CRGBPalette16 palette = getCurrentPalette();

		// Clear the display
		fill_solid(leds, NUM_LEDS, CRGB::Black);

		if (!frame.fft_norm_valid) {
			return;
		}
		
		// Calculate bar width - spread 16 bins across WIDTH
		uint8_t barWidth = WIDTH / 16;
		if (barWidth < 1) barWidth = 1;

		for (uint8_t bin = 0; bin < 16 && bin < NUM_FFT_BINS; bin++) {
			float magnitude = frame.fft_norm[bin];
			uint8_t barHeight = static_cast<uint8_t>(magnitude * HEIGHT);

			// Calculate x position for this bar
			uint8_t xStart = bin * barWidth;

			// Draw the bar from bottom up
			for (uint8_t y = 0; y < barHeight; y++) {
				// Color based on height (low=green, mid=yellow, high=red style via palette)
				uint8_t colorIndex = map(y, 0, HEIGHT - 1, 0, 255);
				CRGB color = ColorFromPalette(gCurrentPalette, colorIndex);

				// Draw bar width
				for (uint8_t xOff = 0; xOff < barWidth; xOff++) {
					uint8_t x = xStart + xOff;
					if (x < WIDTH) {
						// y=0 is top, so draw from bottom: HEIGHT-1-y
						uint16_t idx = xyFunc(x, HEIGHT - 1 - y);
						leds[idx] = color;
					}
				}
			}
		}
	}

	//===============================================================================================
	// VISUALIZATION MODE 1: VU Meter
	// Shows overall volume as horizontal bars filling from left to right
	//===============================================================================================
	
	void drawVUMeter(const myAudio::AudioFrame& frame) {
		//CRGBPalette16 palette = getCurrentPalette();

		// Clear the display
		fill_solid(leds, NUM_LEDS, CRGB::Black);

		// Get RMS (with spike filtering and DC correction)
		float rmsNorm = frame.rms_norm;

		uint8_t level = constrain((int)(rmsNorm * WIDTH), 0, WIDTH);

		// Smooth the level to reduce jitter from occasional spikes
		static uint8_t smoothedLevel = 0;
		// Fast attack, slower decay
		if (level > smoothedLevel) {
			smoothedLevel = level;  // Instant attack
		} else {
			// Decay: blend toward new level
			smoothedLevel = (smoothedLevel * 3 + level) / 4;
		}

		// Draw horizontal bars across the full height
		for (uint8_t x = 0; x < smoothedLevel; x++) {
			// Color based on position (left=green, right=red via palette)
			uint8_t colorIndex = map(x, 0, WIDTH - 1, 0, 255);
			CRGB color = ColorFromPalette(gCurrentPalette, colorIndex);

			for (uint8_t y = 0; y < HEIGHT; y++) {
				uint16_t idx = xyFunc(x, y);
				leds[idx] = color;
			}
		}
	}

	//===============================================================================================
	// VISUALIZATION MODE 2: Beat Pulse
	// Flashes/pulses the entire display on detected beats with decay
	//===============================================================================================
	uint8_t beatBrightness = 0;  // Decaying brightness for beat pulse

	void drawBeatPulse(const myAudio::AudioFrame& frame) {
		//CRGBPalette16 palette = getCurrentPalette();

		// On beat detection, set brightness to max
		if (frame.beat) {
			beatBrightness = 255;
			hue += 32;  // Shift color on each beat
		}

		// Fill with current color at current brightness
		CRGB color = ColorFromPalette(gCurrentPalette, hue);
		color.nscale8(beatBrightness);

		fill_solid(leds, NUM_LEDS, color);

		// Decay the brightness
		if (beatBrightness > 10) {
			beatBrightness = beatBrightness * 0.85;  // Exponential decay
		} else {
			beatBrightness = 0;
		}
	}

	//===============================================================================================
	// VISUALIZATION MODE 3: Bass Ripple
	// Creates expanding rings from center based on bass energy
	//===============================================================================================
	uint8_t rippleRadius = 0;
	uint8_t rippleHue = 0;
	
	void drawBassRipple(const myAudio::AudioFrame& frame) {
		//CRGBPalette16 palette = getCurrentPalette();

		// Fade existing content
		fadeToBlackBy(leds, NUM_LEDS, 30);

		// Get bass level and check for bass hit
		float bass = frame.bass;
	}

	//===============================================================================================
	// VISUALIZATION MODE 4: Beat Detection Example
	//===============================================================================================

	void flBeatDetectionExample(const myAudio::AudioFrame& frame) {
		// Visualize beats on LED strip
		uint32_t timeSinceBeat = frame.timestamp - lastBeatTime;

		if (timeSinceBeat < 100) {
			// Flash bright on beat
			CRGB beatColor = CRGB::Blue; // getBPMColor(frame.bpm);
			fill_solid(leds, NUM_LEDS, beatColor);
		} else if (timeSinceBeat < 200) {
			// Fade out
			CRGB beatColor = CRGB::Blue; // getBPMColor(frame.bpm);
			beatColor.fadeToBlackBy(128);
			fill_solid(leds, NUM_LEDS, beatColor);
		} else {
			// Idle: dim pulse at detected tempo
			if (frame.bpm > 0) {
				// Pulse frequency based on BPM
				float period_ms = (60000.0f / frame.bpm);
				float phase = fmod(static_cast<float>(fl::millis()), period_ms) / period_ms;
				uint8_t brightness = static_cast<uint8_t>((fl::sin(phase * 2.0f * 3.14159f) + 1.0f) * 32.0f);

				CRGB idleColor = CRGB::Blue; // getBPMColor(frame.bpm);
				idleColor.fadeToBlackBy(255 - brightness);
				fill_solid(leds, NUM_LEDS, idleColor);
			} else {
				// No tempo detected yet
				FastLED.clear();
			}
		}
	}

	//===============================================================================================
	// VISUALIZATION MODE 5: Radial Spectrum
	//===============================================================================================
	
	void drawRadialSpectrum(const myAudio::AudioFrame& frame) {

		clearDisplay();
		//CRGBPalette16 palette = getCurrentPalette();

		int centerX = WIDTH / 2;
		int centerY = HEIGHT / 2;

		if (!frame.fft_norm_valid) {
			return;
		}

		for (size_t angle = 0; angle < 360; angle += 6) {  // Reduced resolution
			size_t band = (angle / 6) % NUM_FFT_BINS;

			float magnitude = frame.fft_norm[band];

			int radius = magnitude * (MIN_DIMENSION / 2);

			for (int r = 0; r < radius; r++) {
				float angRad = angle * (PI / 180.0f);
				int16_t x = static_cast<int16_t>(centerX + (r * fl::cosf(angRad)));
				int16_t y = static_cast<int16_t>(centerY + (r * fl::sinf(angRad)));

				if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
					uint8_t colorIndex = fl::map_range<int, uint8_t>(r, 0, radius, 255, 0);
					uint16_t ledIndex = xyFunc(x, y);
					if (ledIndex >= 0 && ledIndex < NUM_LEDS) {
						leds[ledIndex] = ColorFromPalette(gCurrentPalette, colorIndex + hue);
					}
				}
			}
		}
	}

	//===============================================================================================
	// VISUALIZATION MODE 6: Waveform
	//===============================================================================================

	void drawWaveform(const myAudio::AudioFrame& frame) {
		clearDisplay();
		//CRGBPalette16 palette = getCurrentPalette();

		const auto& pcm = frame.pcm;
		if (pcm.size() == 0) {
			return;
		}

		int samplesPerPixel = pcm.size() / WIDTH;
		if (samplesPerPixel < 1) samplesPerPixel = 1;
		int centerY = HEIGHT / 2;

		for (size_t x = 0; x < WIDTH; x++) {
			size_t sampleIndex = x * samplesPerPixel;
			if (sampleIndex >= pcm.size()) break;

			// Get the raw sample value and normalize
			float sample = float(pcm[sampleIndex]) / 32768.0f;  // Normalize to -1.0 to 1.0
			float absSample = fl::fabsf(sample);

			// Log compression for better response to quiet sounds
			float logAmplitude = 0.0f;
			if (absSample > 0.001f) {
				float scaledSample = absSample * 3.0f;
				logAmplitude = fl::log10f(1.0f + scaledSample * 9.0f) / fl::log10f(10.0f);
			}

			// Apply gamma correction
			logAmplitude = fl::powf(logAmplitude, 0.6f);

			// Calculate amplitude in pixels
			int amplitudePixels = int(logAmplitude * (HEIGHT / 2));
			amplitudePixels = fl::clamp(amplitudePixels, 0, HEIGHT / 2);

			// Preserve the sign for proper waveform display
			if (sample < 0) amplitudePixels = -amplitudePixels;

			// Color mapping based on amplitude intensity
			uint8_t colorIndex = fl::map_range<int, uint8_t>(abs(amplitudePixels), 0, HEIGHT/2, 40, 255);
			CRGB color = ColorFromPalette(gCurrentPalette, colorIndex + hue);

			// Apply brightness scaling for low amplitudes
			if (abs(amplitudePixels) < HEIGHT / 4) {
				color.fadeToBlackBy(128 - (abs(amplitudePixels) * 512 / HEIGHT));
			}

			// Draw vertical line from center
			if (amplitudePixels == 0) {
				// Draw center point for zero amplitude
				int ledIndex = xyFunc(x, centerY);
				if (ledIndex >= 0 && ledIndex < NUM_LEDS) {
					leds[ledIndex] = color.fadeToBlackBy(200);
				}
			} else {
				// Draw line from center to amplitude
				int startY = (amplitudePixels > 0) ? centerY : centerY + amplitudePixels;
				int endY = (amplitudePixels > 0) ? centerY + amplitudePixels : centerY;

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


	//===============================================================================================

	void testFunction() {}

	//===============================================================================================

	void runAudioTest() {

		visualizationMode = MODE;
		uint8_t frameMode = visualizationMode;
		const bool needsFft = (frameMode == 0) || (frameMode == 5);
		//const myAudio::AudioFrame& frame = myAudio::getAudioFrame();
		const myAudio::AudioFrame& frame = myAudio::updateAudioFrame(needsFft);

		EVERY_N_MILLISECONDS(40) {
			if (gCurrentPalette != gTargetPalette) {
				nblendPaletteTowardPalette( gCurrentPalette, gTargetPalette, 16); 
			}
		}

		// Run diagnostic mode for calibration testing
		if (DIAGNOSTIC_MODE) {
			myAudio::runAudioDiagnostic();
			// Still run VU meter visualization so you can see audio response on LEDs
			drawVUMeter(frame);
			return;
		}

		if (!frame.valid) {
			return;
		}

		switch(frameMode) {
			case 0:
				drawSpectrum(frame);
				break;
			case 1:
				drawVUMeter(frame);
				break;
			case 2:
				drawBeatPulse(frame);
				break;
			case 3:
				drawBassRipple(frame);
				break;
			case 4:
				flBeatDetectionExample(frame);
				break;
			case 5:
				drawRadialSpectrum(frame);
				break;
			case 6:
				drawWaveform(frame);
				break;
			default:
				drawSpectrum(frame);
				break;
		}

	} // runAudioTest()
		
}  // namespace audioTest
