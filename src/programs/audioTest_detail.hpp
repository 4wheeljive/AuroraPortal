#pragma once
#include "audio/audioProcessing.h"
#include "audio/avHelpers.h"
#include "bleControl.h"

namespace audioTest {

	using namespace myAudio;

	bool audioTestInstance = false;

    uint16_t (*xyFunc)(uint8_t x, uint8_t y);

	//constexpr bool DIAGNOSTIC_MODE = false;

	uint8_t hue = 0;
	uint8_t visualizationMode = 5;  // 0=spectrum, 1=VU meter, 2=beat pulse, 3=bass ripple, 4=flBeatDetection, 5=radial spectrum, 6=waveform, 7=spectrogram, 8=finespectrum, 9=busBeats
	uint8_t fadeSpeed = 20;
	uint8_t currentPaletteNum = 1;
	CRGBPalette16 audioTestPalette = RainbowColors_p;

    void initAudioTest(uint16_t (*xy_func)(uint8_t, uint8_t)) {
        audioTestInstance = true;
        xyFunc = xy_func;
		startingPalette();
	}

	void clearDisplay() {
		if (fadeSpeed == 0) {
			fill_solid(leds, NUM_LEDS, CRGB::Black);
		} else {
			fadeToBlackBy(leds, NUM_LEDS, fadeSpeed);
		}
	}

	//===============================================================================================
	// VISUALIZATION MODE 0: Spectrum Analyzer
	// Shows FFT frequency bins as vertical bars across the matrix
	//===============================================================================================
	
	void drawSpectrum() {
		clearDisplay();
		binConfig& b = maxBins ? bin32 : bin16;
		const myAudio::AudioFrame& frame = myAudio::updateAudioFrame(b);
		if (!frame.valid) {
			return;
		}
		
		// Calculate bar width - spread bins across WIDTH
		uint8_t barWidth = WIDTH / b.NUM_FFT_BINS;
		if (barWidth < 1) barWidth = 1;

		for (uint8_t bin = 0; bin < b.NUM_FFT_BINS; bin++) {
			float magnitude = frame.fft_norm[bin];
			uint8_t barHeight = static_cast<uint8_t>(magnitude * HEIGHT);

			// Calculate x position for this bar
			uint8_t xStart = bin * barWidth;

			// Draw the bar from bottom up
			for (uint8_t y = 0; y < barHeight; y++) {
				// Color based on height (low=green, mid=yellow, high=red style via palette)
				uint8_t colorIndex = map(y, 0, HEIGHT - 1, 0, 255);
				CRGB color = ColorFromPalette(audioTestPalette, colorIndex);

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
	
	void drawVUMeter() {
		clearDisplay();
		binConfig& b = bin16;
		const myAudio::AudioFrame& frame = myAudio::updateAudioFrame(b);
		if (!frame.valid) {
			return;
		}

		// Get RMS (with spike filtering and DC correction)
		float rmsNorm = frame.rms_norm;

		// frame.rms_norm is really low

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
			CRGB color = ColorFromPalette(audioTestPalette, colorIndex);

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

	void drawBeatPulse() {
		
		/*
		//clearDisplay();
		binConfig& b = bin16;
		const myAudio::AudioFrame& frame = myAudio::updateAudioFrame(b);
		if (!frame.valid) {
			return;
		}
		// On beat detection, set brightness to max
		if (frame.beat) {
			beatBrightness = 255;
			hue += 32;  // Shift color on each beat
		}

		// Fill with current color at current brightness
		CRGB color = ColorFromPalette(audioTestPalette, hue);
		color.nscale8(beatBrightness);

		fill_solid(leds, NUM_LEDS, color);

		// Decay the brightness
		if (beatBrightness > 10) {
			beatBrightness = beatBrightness * 0.85;  // Exponential decay
		} else {
			beatBrightness = 0;
		}
			*/
		
	}

	//===============================================================================================
	// VISUALIZATION MODE 3: Bass Ripple
	// Creates expanding rings from center based on bass energy
	//===============================================================================================
	uint8_t rippleRadius = 0;
	uint8_t rippleHue = 0;
	
	void drawBassRipple() {
		//clearDisplay();
		binConfig& b = maxBins ? bin32 : bin16;
		const myAudio::AudioFrame& frame = myAudio::updateAudioFrame(b);
		if (!frame.valid) {
			return;
		}

		// NEED TO ADD SOME VISUALIZER LOGIC!!!

		// Fade existing content
		fadeToBlackBy(leds, NUM_LEDS, 30);

	}

	//===============================================================================================
	// VISUALIZATION MODE 4: Beat Detection Example
	//===============================================================================================

	void flBeatDetectionExample() {
	
	}

	//===============================================================================================
	// VISUALIZATION MODE 5: Radial Spectrum
	//===============================================================================================
	
	void drawRadialSpectrum() {
		
		clearDisplay();
		
		binConfig& b = maxBins ? bin32 : bin16;
		const myAudio::AudioFrame& frame = myAudio::updateAudioFrame(b);
		if (!frame.valid) { return; }
		
		int centerX = WIDTH / 2;
		int centerY = HEIGHT / 2;

		for (size_t angle = 0; angle < 360; angle += 2) {  // Reduced resolution
			size_t band = (angle / 2) % b.NUM_FFT_BINS;

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
						leds[ledIndex] = ColorFromPalette(audioTestPalette, colorIndex + hue);
					}
				}
			}
		}
	}

	//===============================================================================================
	// VISUALIZATION MODE 6: Waveform
	//===============================================================================================

	void drawWaveform() {
		clearDisplay();
		binConfig& b = maxBins ? bin32 : bin16;
		const myAudio::AudioFrame& frame = myAudio::updateAudioFrame(b);
		if (!frame.valid) { return;	}

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
			CRGB color = ColorFromPalette(audioTestPalette, colorIndex + hue);

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
	// VISUALIZATION MODE 7: Spectrogram Waterfall
	// Shows FFT energy across width, scrolling over time to reveal dynamic range
	//===============================================================================================

	void drawSpectrogram() {
		binConfig& b = maxBins ? bin32 : bin16;
		const myAudio::AudioFrame& frame = myAudio::updateAudioFrame(b);
		
		if (!frame.valid) { return;	}

		// Scroll existing image down by 1 row (time axis)
		for (int y = HEIGHT - 1; y > 0; y--) {
			for (int x = 0; x < WIDTH; x++) {
				uint16_t dst = xyFunc(x, y);
				uint16_t src = xyFunc(x, y - 1);
				leds[dst] = leds[src];
			}
		}

		// Draw newest FFT slice at top row
		for (int x = 0; x < WIDTH; x++) {
			float pos = (WIDTH > 1) ? (float)x * (b.NUM_FFT_BINS - 1) / (float)(WIDTH - 1) : 0.0f;
			int bin0 = (int)pos;
			int bin1 = (bin0 + 1 < b.NUM_FFT_BINS) ? (bin0 + 1) : bin0;
			float t = pos - (float)bin0;

			float mag = frame.fft_norm[bin0] * (1.0f - t) + frame.fft_norm[bin1] * t;

			// Emphasize dynamic range: soft floor + log compression + gamma
			mag = FL_MAX(0.0f, mag - 0.01f);
			float magLog = fl::log10f(1.0f + mag * 9.0f) / fl::log10f(10.0f);
			float magGamma = fl::powf(magLog, 0.7f);

			uint8_t colorIndex = (uint8_t)fl::clamp(magGamma * 255.0f, 0.0f, 255.0f);
			CRGB color = ColorFromPalette(audioTestPalette, colorIndex);
			color.nscale8((uint8_t)fl::clamp(magGamma * 255.0f, 0.0f, 255.0f));

			uint16_t idx = xyFunc(x, 0);
			leds[idx] = color;
		}
	}

	//===============================================================================================
	// VISUALIZATION MODE 8: Fine Spectrum Bars
	// Interpolated FFT across width with log/gamma for smoother dynamic range
	//===============================================================================================

	void drawFineSpectrumBars() {
		
		binConfig& b = maxBins ? bin32 : bin16;
		const myAudio::AudioFrame& frame = myAudio::updateAudioFrame(b);

		if (!frame.valid) { return;	}

		// Fade slightly for smoother motion
		fadeToBlackBy(leds, NUM_LEDS, 40);

		for (int x = 0; x < WIDTH; x++) {
			float pos = (WIDTH > 1) ? (float)x * (b.NUM_FFT_BINS - 1) / (float)(WIDTH - 1) : 0.0f;
			int bin0 = (int)pos;
			int bin1 = (bin0 + 1 < b.NUM_FFT_BINS) ? (bin0 + 1) : bin0;
			float t = pos - (float)bin0;

			float mag = frame.fft_norm[bin0] * (1.0f - t) + frame.fft_norm[bin1] * t;

			// Emphasize dynamic range
			mag = FL_MAX(0.0f, mag - 0.01f);
			float magLog = fl::log10f(1.0f + mag * 9.0f) / fl::log10f(10.0f);
			float magGamma = fl::powf(magLog, 0.7f);

			int barHeight = (int)fl::clamp(magGamma * (float)HEIGHT, 0.0f, (float)HEIGHT);
			uint8_t colorIndex = (uint8_t)fl::clamp(magGamma * 255.0f, 0.0f, 255.0f);
			CRGB color = ColorFromPalette(audioTestPalette, colorIndex);

			for (int y = 0; y < barHeight; y++) {
				uint16_t idx = xyFunc(x, HEIGHT - 1 - y);
				leds[idx] = color;
			}
		}
	}

	//===============================================================================================
	// VISUALIZATION MODE 9: Bus Beat Monitor
	// Horizontal bars showing per-bus energy level and beat detection flashes
	//===============================================================================================

	void drawBusBeats() {
		clearDisplay();
		binConfig& b = maxBins ? bin32 : bin16;
		const myAudio::AudioFrame& frame = myAudio::updateAudioFrame(b);
		if (!frame.valid) return;

		uint32_t now = frame.timestamp;
		bool gateOpen = myAudio::noiseGateOpen;

		// Always call dynamicPulse so ramps decay naturally even during silence
		dynamicPulse(myAudio::busA, now);
		dynamicPulse(myAudio::busB, now);
		dynamicPulse(myAudio::busC, now);

		const uint8_t barCols = (uint8_t)(WIDTH * 0.75f);

		// Draw one bus row-band: level meter on the left, beat flash on the right
		auto drawBus = [&](myAudio::Bus& bus, uint8_t startRow, uint8_t busHue) {
			// Level meter suppressed when gate is closed (gain calibration can amplify noise)
			float level = gateOpen ? bus._norm : 0.0f;
			uint8_t filledCols = (uint8_t)(level * barCols);

			for (uint8_t x = 0; x < filledCols; x++) {
				uint8_t bri = 80 + (uint8_t)((float)x / (float)barCols * 175.0f);
				CRGB color = CHSV(busHue, 255, bri);
				for (uint8_t row = 0; row < BUS_ROWS; row++) {
					uint8_t y = startRow + row;
					if (y < HEIGHT) {
						leds[xyFunc(x, y)] = color;
					}
				}
			}

			// Beat flash zone: barCols to WIDTH-1 (suppressed when gate is closed)
			uint8_t flash = gateOpen ? (uint8_t)(fl::clamp(bus.avResponse, 0.0f, 1.0f) * 255) : 0;
			if (flash > 0) {
				CRGB flashColor = CHSV(busHue, 150, flash);
				for (uint8_t x = barCols; x < WIDTH; x++) {
					for (uint8_t row = 0; row < BUS_ROWS; row++) {
						uint8_t y = startRow + row;
						if (y < HEIGHT) {
							leds[xyFunc(x, y)] = flashColor;
						}
					}
				}
			}
		};

		drawBus(myAudio::busA, 0,              0);    // Red   (bass)
		drawBus(myAudio::busB, BUS_ROWS,       96);   // Green (mid)
		drawBus(myAudio::busC, 2 * BUS_ROWS,   160);  // Blue  (treble)
	}

	//===============================================================================================

	void runAudioTest() {

		EVERY_N_MILLISECONDS(1000) {
			printDiagnostics();
		}

    	EVERY_N_MILLISECONDS(40) {
			if (gCurrentPalette != gTargetPalette) {
				nblendPaletteTowardPalette( gCurrentPalette, gTargetPalette, 16); 
			}
		}

		switch(MODE) {
			case 0:
				drawSpectrum();
				break;
			case 1:
				drawVUMeter();
				break;
			case 2:
				drawBeatPulse();
				break;
			case 3:
				drawBassRipple();
				break;
			case 4:
				flBeatDetectionExample();
				break;
			case 5:
				drawRadialSpectrum();
				break;
			case 6:
				drawWaveform();
				break;
			case 7:
				drawSpectrogram();
				break;
			case 8:
				drawFineSpectrumBars();
				break;
			case 9:
				drawBusBeats();
				break;
			default:
				drawSpectrum();
				break;
		}

	} // runAudioTest()
		
}  // namespace audioTest
