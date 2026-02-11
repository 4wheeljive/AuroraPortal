#pragma once

#include "fl/audio.h"
#include "fl/fft.h"
#include "fl/audio/audio_context.h"
#include "fl/fx/audio/audio_processor.h"
#include "fl/math_macros.h"
#include "audioInput.h"
#include "bleControl.h"
#include "beatDetector.h"

namespace myAudio {

    using namespace fl;

    //=========================================================================
    // Core audio objects
    //=========================================================================

    AudioSample currentSample;      // Raw sample from I2S (kept for diagnostics)
    AudioSample filteredSample;     // Spike-filtered sample for processing
    AudioProcessor audioProcessor;
    bool audioProcessingInitialized = false;

    // Custom beat detector (testing)
    BeatTracker beatTracker;

    // Buffer for filtered PCM data
    int16_t filteredPcmBuffer[512];  // Matches I2S_AUDIO_BUFFER_LEN

    //=========================================================================
    // State populated by callbacks (reactive approach)
    //=========================================================================

    // Detector variables
    bool beatDetected = false;
    uint32_t lastBeatTime = 0;
    uint32_t beatCount = 0;
    uint32_t onsetCount = 0;
    float lastOnsetStrength = 0.0f;
    bool noiseGateOpen = false;

    //=========================================================================
    // FFT configuration
    //=========================================================================

    // Maximum FFT bins (compile-time constant for array sizing)
    constexpr uint8_t MAX_FFT_BINS = 32;

    constexpr float FFT_MIN_FREQ = 20.0f;    
    constexpr float FFT_MAX_FREQ = 16000.f;
 
    struct binConfig {
        //BinConfig binSize;
        uint8_t NUM_FFT_BINS;
        uint8_t firstMidBin;
        uint8_t firstTrebleBin;
        uint8_t bassBins;
        uint8_t midBins;
        uint8_t trebleBins;
        float fBassBins; // for use as divisor in averaging calculations
        float fMidBins;
        float fTrebleBins; 
    };

    binConfig bin16;
    binConfig bin32;

    void setBinConfig() {
        
        // 16-bin: log spacing 20–16000 Hz
        // Bass: bins 0–5   (20–245 Hz)    6 bins
        // Mid:  bins 6–10  (245–1976 Hz)   5 bins
        // Tre:  bins 11–15 (1976–16000 Hz) 5 bins
        bin16.NUM_FFT_BINS = 16;
        bin16.firstMidBin = 6;
        bin16.firstTrebleBin = 11;
        bin16.bassBins = bin16.firstMidBin;
        bin16.midBins = bin16.firstTrebleBin - bin16.firstMidBin;
        bin16.trebleBins = bin16.NUM_FFT_BINS - bin16.firstTrebleBin;
        bin16.fBassBins = float(bin16.bassBins);
        bin16.fMidBins = float(bin16.midBins);
        bin16.fTrebleBins = float(bin16.trebleBins);

        // 32-bin: log spacing 20–16000 Hz
        // Bass: bins 0–11  (20–245 Hz)    12 bins
        // Mid:  bins 12–21 (245–1979 Hz)  10 bins
        // Tre:  bins 22–31 (1979–16000 Hz) 10 bins
        bin32.NUM_FFT_BINS = 32;
        bin32.firstMidBin = 12;
        bin32.firstTrebleBin = 22;
        bin32.bassBins = bin32.firstMidBin;
        bin32.midBins = bin32.firstTrebleBin - bin32.firstMidBin;
        bin32.trebleBins = bin32.NUM_FFT_BINS - bin32.firstTrebleBin;
        bin32.fBassBins = float(bin32.bassBins);
        bin32.fMidBins = float(bin32.midBins);
        bin32.fTrebleBins = float(bin32.trebleBins);

    }

    //=========================================================================
    // Visualization normalization & auto-calibration
    //=========================================================================

    // Scale factors: single user control → domain-specific internal values
    // Level (RMS) values are tiny (~0.001-0.02), FFT/dB values are larger (~0.1-0.8)
    constexpr float FLOOR_SCALE_LEVEL = 0.05f;
    constexpr float FLOOR_SCALE_FFT   = 0.3f;
    constexpr float GAIN_SCALE_LEVEL  = 12.0f;
    constexpr float GAIN_SCALE_FFT    = 8.0f;

    struct AudioVizConfig {
        // Internal values (derived from single user controls via scale factors)
        float audioFloorLevel = 0.0f;
        float audioFloorFft = 0.0f;
        float gainLevel = GAIN_SCALE_LEVEL;
        float gainFft = GAIN_SCALE_FFT ;

        bool autoGain = true;
        //float autoGainTarget = 0.7f;  // old: mean-targeting
        float autoGainTarget = 0.5f;   // new: P90 ceiling → rms_norm target for loud signals
        bool autoFloor = true;
        float autoFloorAlpha = 0.01f;
        float autoFloorMin = 0.0f;
        float autoFloorMax = 0.5f;

        float bpmScaleFactor = 0.5f;
        float bpmMinValid = 50.0f;
        float bpmMaxValid = 250.0f;
    };

    AudioVizConfig vizConfig;
    float autoGainValue = 1.0f;

    inline void updateVizConfig() {
        // Map single user controls to domain-specific internal values
        vizConfig.audioFloorLevel = cAudioFloor * FLOOR_SCALE_LEVEL;
        vizConfig.audioFloorFft   = cAudioFloor * FLOOR_SCALE_FFT;
        vizConfig.gainLevel       = cAudioGain * GAIN_SCALE_LEVEL;
        vizConfig.gainFft         = cAudioGain * GAIN_SCALE_FFT;
        vizConfig.autoGainTarget  = cAutoGainTarget;
        vizConfig.autoFloorAlpha  = cAutoFloorAlpha;
        vizConfig.autoFloorMin    = cAutoFloorMin;
        vizConfig.autoFloorMax    = cAutoFloorMax;
        vizConfig.autoGain        = autoGain;
        vizConfig.autoFloor       = autoFloor;
        vizConfig.bpmScaleFactor  = cBpmScaleFactor;
    }

    void updateAutoGain(float level) {
        if (!vizConfig.autoGain) {
            autoGainValue = 1.0f;
            return;
        }

        // --- OLD: Mean-targeting approach (compresses dynamic range) ---
        // static float avgLevel = 0.0f;
        // avgLevel = avgLevel * 0.95f + level * 0.05f;
        //
        // if (avgLevel > 0.01f) {
        //     float gainAdjust = vizConfig.autoGainTarget / avgLevel;
        //     gainAdjust = fl::clamp(gainAdjust, 0.5f, 2.0f);
        //     autoGainValue = autoGainValue * 0.9f + gainAdjust * 0.1f;
        // }

        // --- NEW: Percentile-ceiling approach (preserves dynamic range) ---
        // Estimate the 90th percentile of recent input levels using
        // Robbins-Monro stochastic quantile estimation. Gain is set so
        // the ceiling maps to autoGainTarget in the final rms_norm output,
        // while quieter signals remain proportionally lower.

        static float ceilingEstimate = 0.02f;  // initial guess for P90 of rmsNormRaw

        constexpr float targetPercentile = 0.90f;
        constexpr float alpha = 0.005f;  // ~2-4 sec half-life at 60fps

        // Asymmetric proportional update: converges to the target quantile
        if (level > ceilingEstimate) {
            ceilingEstimate += alpha * (1.0f - targetPercentile) * (level - ceilingEstimate);
        } else {
            ceilingEstimate += alpha * targetPercentile * (level - ceilingEstimate);
        }
        ceilingEstimate = FL_MAX(ceilingEstimate, 0.0005f);  // prevent collapse

        // Solve for autoGainValue:
        //   rms_norm ≈ ceilingEstimate × gainLevel × autoGainValue = autoGainTarget
        float desired = vizConfig.autoGainTarget / (ceilingEstimate * vizConfig.gainLevel);
        desired = fl::clamp(desired, 0.1f, 8.0f);

        // Smooth the transition to avoid abrupt gain jumps
        autoGainValue = autoGainValue * 0.95f + desired * 0.05f;
    }

    void updateAutoFloor(float level) {
        if (!vizConfig.autoFloor) {
            return;
        }

        // Only adapt when near the existing floor to avoid chasing loud signals.
        if (level < (vizConfig.audioFloorLevel + 0.02f)) {
            float nf = vizConfig.audioFloorLevel * (1.0f - vizConfig.autoFloorAlpha)
                       + level * vizConfig.autoFloorAlpha;
            vizConfig.audioFloorLevel = fl::clamp(nf, vizConfig.autoFloorMin, vizConfig.autoFloorMax);
        }
    }

    //=========================================================================
    // Initialize audio processing with callbacks
    //=========================================================================

    void initAudioProcessing() {

        if (audioProcessingInitialized) { return; }

        // Initialize bin configurations
        setBinConfig();
        beatTracker.reset();

        Serial.println("AudioProcessor initialized with callbacks");
        audioProcessingInitialized = true;
    }

    //=========================================================================
    // Sample audio and process
    //=========================================================================

    void sampleAudio() {

        if (!audioSource) {
            beatDetected = false;
            currentSample = AudioSample();
            filteredSample = AudioSample();
            return;
        }

        checkAudioInput();

        fl::string errorMsg;
        if (audioSource->error(&errorMsg)) {
            Serial.print("Audio error: ");
            Serial.println(errorMsg.c_str());
            beatDetected = false;
            currentSample = AudioSample();
            filteredSample = AudioSample();
            return;
        }

        // Reset per-frame state
        beatDetected = false;

        // Read audio sample from I2S
        currentSample = audioSource->read();

        // DIAGNOSTIC: Check sample validity and raw data
        static uint32_t sampleCount = 0;
        static uint32_t validCount = 0;
        static uint32_t invalidCount = 0;
        sampleCount++;

        if (!currentSample.isValid()) {
            invalidCount++;
            EVERY_N_MILLISECONDS(500) {
                Serial.print("[DIAG] Sample INVALID. Total: ");
                Serial.print(sampleCount);
                Serial.print(" Valid: ");
                Serial.print(validCount);
                Serial.print(" Invalid: ");
                Serial.println(invalidCount);
            }
            return;
        }

        validCount++;

        //=====================================================================
        // SPIKE FILTERING - Filter raw samples BEFORE AudioProcessor
        // This ensures beat detection, FFT, bass/mid/treble all get clean data
        //=====================================================================

        // Spike filtering: I2S occasionally produces spurious samples near int16_t max/min
        constexpr int16_t SPIKE_THRESHOLD = 10000;  // Samples beyond this are glitches

        auto rawPcm = currentSample.pcm();
        size_t n = rawPcm.size();
        size_t nClamped = (n > 512) ? 512 : n;
       
        // Calculate DC offset from non-spike samples
        int64_t dcSum = 0;
        size_t dcCount = 0;
        for (size_t i = 0; i < n; i++) {
            if (rawPcm[i] > -SPIKE_THRESHOLD && rawPcm[i] < SPIKE_THRESHOLD) {
                dcSum += rawPcm[i];
                dcCount++;
            }
        }
        int16_t dcOffset = (dcCount > 0) ? static_cast<int16_t>(dcSum / dcCount) : 0;

        // Copy samples to filtered buffer, replacing spikes with zero
        // Also calculate RMS for noise gate decision
        uint64_t sumSq = 0;
        size_t validSamples = 0;

        for (size_t i = 0; i < nClamped; i++) {
            if (rawPcm[i] > -SPIKE_THRESHOLD && rawPcm[i] < SPIKE_THRESHOLD) {
                // Valid sample - keep it (with DC correction applied)
                int16_t corrected = rawPcm[i] - dcOffset;
                filteredPcmBuffer[i] = corrected;
                sumSq += static_cast<int32_t>(corrected) * corrected;
                validSamples++;
            } else {
                // Spike detected - replace with zero
                filteredPcmBuffer[i] = 0;
            }
        }

        // Calculate RMS of the filtered signal
        float blockRMS = (validSamples > 0) ? fl::sqrtf(static_cast<float>(sumSq) / validSamples) : 0.0f;

        // NOISE GATE with hysteresis to prevent flickering
        // Gate opens when signal exceeds NOISE_GATE_OPEN
        // Gate closes when signal falls below NOISE_GATE_CLOSE
        if (blockRMS >= cNoiseGateOpen) {
            noiseGateOpen = true;
        } else if (blockRMS < cNoiseGateClose) {
            noiseGateOpen = false;
        }
        // Between CLOSE and OPEN thresholds, gate maintains its previous state

        // If gate is closed, zero out entire buffer
        if (!noiseGateOpen) {
            for (size_t i = 0; i < nClamped; i++) {
                filteredPcmBuffer[i] = 0;
            }
        }

        // Create filtered AudioSample from the cleaned buffer
        fl::span<const int16_t> filteredSpan(filteredPcmBuffer, nClamped);
        filteredSample = AudioSample(filteredSpan, currentSample.timestamp());

        // Process through AudioProcessor (triggers callbacks)
        audioProcessor.update(filteredSample);  // filtered
    
    } // sampleAudio()

    //===============================================================================

    // Get the AudioContext for direct FFT access
    fl::shared_ptr<AudioContext> getContext() {
        return audioProcessor.getContext();
    }
   
    // Get RMS from the filtered sample
    // This function just adds temporal smoothing for stability
    float getRMS() {
        if (!filteredSample.isValid()) return 0.0f;

        // Get RMS directly from the pre-filtered sample
        float rms = filteredSample.rms();

        // Temporal smoothing using median filter to reject occasional outliers
        static float history[4] = {0, 0, 0, 0};
        static uint8_t histIdx = 0;

        // Store current value in history
        history[histIdx] = rms;
        histIdx = (histIdx + 1) % 4;

        // Find median of last 4 values (simple sort for 4 elements)
        float sorted[4];
        for (int i = 0; i < 4; i++) sorted[i] = history[i];
        for (int i = 0; i < 3; i++) {
            for (int j = i + 1; j < 4; j++) {
                if (sorted[i] > sorted[j]) {
                    float tmp = sorted[i];
                    sorted[i] = sorted[j];
                    sorted[j] = tmp;
                }
            }
        }
        // Median is average of middle two values
        float median = (sorted[1] + sorted[2]) / 2.0f;

        // Additional smoothing on the median
        static float smoothedRMS = 0.0f;
        if (median > smoothedRMS) {
            // Moderate attack: 70% new, 30% old [was 50/50]
            smoothedRMS = median * 0.7f + smoothedRMS * 0.3f;
        } else {
            // Moderate decay: 70% new, 30% old
            smoothedRMS = median * 0.7f + smoothedRMS * 0.3f;
        }

        return smoothedRMS;
    } // getRMS()
    
    // Get filtered PCM data for waveform visualization
    fl::Slice<const int16_t> getPCM() {
        return filteredSample.pcm();
    }

    // Get raw (unfiltered) PCM data - for diagnostics only
    fl::Slice<const int16_t> getRawPCM() {
        return currentSample.pcm();
    }



    // ---------------------------------------------------------------------------------
    // FFT --------------------

    // Unified FFT path (avoids AudioContext stack usage)
    // Uses static FFT storage and explicit args for consistent bins.
    static fl::FFTBins fftBins(myAudio::MAX_FFT_BINS);
    static fl::FFT fftEngine;

    const fl::FFTBins* getFFT(binConfig& b) {
        if (!filteredSample.isValid()) return nullptr;

        const auto &pcm = filteredSample.pcm();
        if (pcm.size() == 0) return nullptr;

        int sampleRate = fl::FFT_Args::DefaultSampleRate();
        if (config.is<fl::AudioConfigI2S>()) {
            sampleRate = static_cast<int>(config.get<fl::AudioConfigI2S>().mSampleRate);
        } else if (config.is<fl::AudioConfigPdm>()) {
            sampleRate = static_cast<int>(config.get<fl::AudioConfigPdm>().mSampleRate);
        }

        fl::FFT_Args args(
            static_cast<int>(pcm.size()),
            b.NUM_FFT_BINS,
            FFT_MIN_FREQ,
            FFT_MAX_FREQ,
            sampleRate
        );

        fl::span<const fl::i16> span(pcm.data(), pcm.size());
        fftEngine.run(span, &fftBins, args);
        return &fftBins;
    }

    const fl::FFTBins* getFFT_direct(binConfig& b) {
        return getFFT(b);
    }

    //=====================================================================
    // Per-frame snapshot helper
    // Ensures one audio sample -> one set of derived values per frame.
    //=====================================================================

    struct AudioFrame {
        bool valid = false;
        uint32_t timestamp = 0;
        bool beat = false;
        float bpm = 0.0f;
        //float bpmIndex = 1.0f;
        float rms_raw = 0.0f;
        float rms = 0.0f;
        float rms_norm = 0.0f;
        float rms_factor = 0.0f;
        float rms_fast_norm = 0.0f;
        float bass_norm = 0.0f;
        float bass_factor = 0.0f;
        float mid_norm = 0.0f;
        float mid_factor = 0.0f;
        float treble_norm = 0.0f;
        float treble_factor = 0.0f;
        float energy = 0.0f;
        float peak = 0.0f;
        float peak_norm = 0.0f;
        const fl::FFTBins* fft = nullptr;
        bool fft_norm_valid = false;
        float fft_norm[MAX_FFT_BINS] = {0};
        fl::Slice<const int16_t> pcm;
    };


    inline const AudioFrame& beginAudioFrame(binConfig& b) {
        static AudioFrame frame;
        static uint32_t lastFftTimestamp = 0;

        updateVizConfig();
        sampleAudio();

        frame.valid = filteredSample.isValid();
        frame.timestamp = currentSample.timestamp();
        
        frame.pcm = filteredSample.pcm();

        // Run FFT engine once per timestamp
        static const fl::FFTBins* lastFft = nullptr;
        const fl::FFTBins* fftForBeat = nullptr;
        float rmsNormFast = 0.0f;
        float timeEnergy = 0.0f;
        float beatBins[MAX_FFT_BINS] = {0.0f};
        bool beatBinsValid = false;
        if (frame.valid) {
            if (frame.timestamp != lastFftTimestamp) {
                fftForBeat = getFFT(b);
                lastFftTimestamp = frame.timestamp;
                lastFft = fftForBeat;
            } else {
                fftForBeat = lastFft;
            }
            frame.rms_raw = filteredSample.rms(); // no temporal smoothing
            rmsNormFast = frame.rms_raw / 32768.0f;
            rmsNormFast = fl::clamp(rmsNormFast, 0.0f, 1.0f);
        } else {
            frame.rms_raw = 0.0f;
        }
        frame.fft = fftForBeat;

        frame.rms = getRMS(); // with temporal smoothing

        if (frame.valid) {
        // Normalize RMS/peak and update auto-calibration
        float rmsNormRaw = frame.rms / 32768.0f; // with temporal smoothing
        rmsNormFast = frame.rms_raw / 32768.0f; // no temporal smoothing
        float peakNormRaw = frame.peak / 32768.0f;
        rmsNormRaw = fl::clamp(rmsNormRaw, 0.0f, 1.0f);
        rmsNormFast = fl::clamp(rmsNormFast, 0.0f, 1.0f);
        peakNormRaw = fl::clamp(peakNormRaw, 0.0f, 1.0f);
        
        updateAutoFloor(rmsNormRaw);
        updateAutoGain(rmsNormRaw);

        timeEnergy = FL_MAX(0.0f, rmsNormFast - vizConfig.audioFloorLevel);

        float gainAppliedLevel = vizConfig.gainLevel * autoGainValue;
        float gainAppliedFft = vizConfig.gainFft * autoGainValue;
        
        frame.rms_norm = rmsNormRaw;
        frame.rms_norm = fl::clamp(FL_MAX(0.0f, frame.rms_norm - vizConfig.audioFloorLevel) * gainAppliedLevel, 0.0f, 1.0f);

        // rmsFactor: 0.0–2.0 multiplicative scale, 1.0 at neutralPoint
        constexpr float neutralPoint = 0.3f;
        constexpr float gamma = 0.5754f; // ln(0.5)/ln(0.3)
        frame.rms_factor = 2.0f * fl::powf(frame.rms_norm, gamma);
        
        frame.rms_fast_norm = rmsNormFast;
        frame.rms_fast_norm = fl::clamp(FL_MAX(0.0f, frame.rms_fast_norm - vizConfig.audioFloorLevel) * gainAppliedLevel, 0.0f, 1.0f);

        frame.peak_norm = peakNormRaw;
        frame.peak_norm = fl::clamp(FL_MAX(0.0f, frame.peak_norm - vizConfig.audioFloorLevel) * gainAppliedLevel, 0.0f, 1.0f);

        // Derive bands from FFT bins (band boundaries set in binConfig)
        frame.fft_norm_valid = false;
        if (frame.fft && frame.fft->bins_db.size() > 0) {
            // First pass: compute pre-gain values for band calculation
            float preGainBins[MAX_FFT_BINS];
            for (uint8_t i = 0; i < b.NUM_FFT_BINS; i++) {
                float mag = 0.0f;
                if (i < frame.fft->bins_db.size()) {
                    mag = frame.fft->bins_db[i] / 100.0f;  // normalize dB bins
                }
                mag = FL_MAX(0.0f, mag - vizConfig.audioFloorFft);
                preGainBins[i] = fl::clamp(mag, 0.0f, 1.0f);

                // Apply visual gain for spectrum displays
                frame.fft_norm[i] = fl::clamp(mag * gainAppliedFft, 0.0f, 1.0f);
            }
            frame.fft_norm_valid = true;

            // Derive bass/mid/treble from PRE-GAIN FFT bins
            // This ensures bands track actual signal level, not visual gain
            float bassSum = 0.0f, midSum = 0.0f, trebleSum = 0.0f;
            for (uint8_t i = 0; i < b.firstMidBin; i++) bassSum += preGainBins[i];
            for (uint8_t i = b.firstMidBin; i < b.firstTrebleBin; i++) midSum += preGainBins[i];
            for (uint8_t i = b.firstTrebleBin; i < b.NUM_FFT_BINS; i++) trebleSum += preGainBins[i];

            // Average each band, then scale so bands track with RMS-based visuals
            float bassAvg = bassSum / b.fBassBins;
            float midAvg = midSum / b.fMidBins;
            float trebleAvg = trebleSum / b.fTrebleBins;

            // --- NEW: Cross-domain calibration with per-band spectral EQ ---
            // Per-band EMA tracks each band's typical level. Dividing by
            // the running average flattens the natural spectral tilt (mid
            // dominance) so each band independently reports activity
            // relative to its own baseline. The cross-calibration ratio
            // then maps the flattened values onto the RMS-domain scale.
            static float avgBassLevel = 0.3f;
            static float avgMidLevel = 0.3f;
            static float avgTreLevel = 0.3f;
            static float crossCalRatio = 0.05f;

            constexpr float eqAlpha = 0.02f;  // ~1-2 sec half-life
            avgBassLevel += eqAlpha * (bassAvg - avgBassLevel);
            avgMidLevel  += eqAlpha * (midAvg  - avgMidLevel);
            avgTreLevel  += eqAlpha * (trebleAvg - avgTreLevel);
            avgBassLevel = FL_MAX(avgBassLevel, 0.01f);
            avgMidLevel  = FL_MAX(avgMidLevel,  0.01f);
            avgTreLevel  = FL_MAX(avgTreLevel,  0.01f);

            // Beat bins: floor-subtracted, lightly band-leveled, auto-gain applied
            float beatGain = fl::clamp(autoGainValue, 0.5f, 3.0f);
            for (uint8_t i = 0; i < b.NUM_FFT_BINS; i++) {
                float bandLevel = (i < b.firstMidBin) ? avgBassLevel
                                  : (i < b.firstTrebleBin) ? avgMidLevel
                                  : avgTreLevel;
                float bandEq = 1.0f / bandLevel;
                bandEq = fl::clamp(bandEq, 0.6f, 1.8f);
                beatBins[i] = fl::clamp(preGainBins[i] * bandEq * beatGain, 0.0f, 1.0f);
            }
            beatBinsValid = true;

            // Flatten: normalize each band to its own typical level
            float bassFlat = bassAvg / avgBassLevel;
            float midFlat  = midAvg  / avgMidLevel;
            float treFlat  = trebleAvg / avgTreLevel;

            // Cross-calibrate flattened values to RMS domain
            float rmsPostFloor = FL_MAX(0.0f, rmsNormRaw - vizConfig.audioFloorLevel);
            float avgFlatBand = (bassFlat + midFlat + treFlat) / 3.0f;
            if (avgFlatBand > 0.1f && rmsPostFloor > 0.0005f) {
                float instantRatio = rmsPostFloor / avgFlatBand;
                crossCalRatio = crossCalRatio * 0.95f + instantRatio * 0.05f;
            }

            frame.bass_norm = fl::clamp(bassFlat * crossCalRatio * gainAppliedLevel, 0.0f, 1.0f);
            frame.mid_norm  = fl::clamp(midFlat  * crossCalRatio * gainAppliedLevel, 0.0f, 1.0f);
            frame.treble_norm = fl::clamp(treFlat * crossCalRatio * gainAppliedLevel, 0.0f, 1.0f);

            // Band factors: same power curve as rms_factor
            frame.bass_factor = 2.0f * fl::powf(frame.bass_norm, gamma);
            frame.mid_factor  = 2.0f * fl::powf(frame.mid_norm, gamma);
            frame.treble_factor = 2.0f * fl::powf(frame.treble_norm, gamma);
        } else {
            for (uint8_t i = 0; i < b.NUM_FFT_BINS; i++) {
                frame.fft_norm[i] = 0.0f;
            }
            frame.fft_norm_valid = false;
            // Without FFT, fall back to RMS-scaled approximation
            frame.bass_norm = frame.rms_norm;
            frame.bass_factor = frame.rms_factor;
            frame.mid_norm = frame.rms_norm;
            frame.mid_factor = frame.rms_factor;
            frame.treble_norm = frame.rms_norm;
            frame.treble_factor = frame.rms_factor;
        }
        } else {
            frame.fft = nullptr;
            frame.fft_norm_valid = false;
            frame.rms_norm = 0.0f;
            frame.peak_norm = 0.0f;
            frame.bass_norm = 0.0f;
            frame.bass_factor = 0.0f;
            frame.mid_norm = 0.0f;
            frame.mid_factor = 0.0f;
            frame.treble_norm = 0.0f;
            frame.treble_factor = 0.0f;
            for (uint8_t i = 0; i < b.NUM_FFT_BINS; i++) {
                frame.fft_norm[i] = 0.0f;
            }
        }

        // Beat tracking using preprocessed FFT bins (and time-domain energy)
        beatTracker.setConfig(
            cMinBpm,
            cMaxBpm,
            cBassWeight,
            cMidWeight,
            cTrebleWeight,
            cOdfSmoothAlpha,
            cOdfMeanAlpha,
            cThreshStdMult,
            cMinOdfThreshold,
            cTempoUpdateInterval,
            cTempoSmoothAlpha
        );
        beatTracker.update(
            fftForBeat,
            nullptr,
            b.NUM_FFT_BINS,
            b.firstMidBin,
            b.firstTrebleBin,
            frame.timestamp,
            vizConfig.audioFloorFft,
            noiseGateOpen && frame.valid,
            timeEnergy
        );

        frame.beat = beatTracker.isBeat();
        beatDetected = frame.beat;
        lastBeatTime = beatTracker.getLastBeatTime();
        beatCount = beatTracker.getBeatCount();
        lastOnsetStrength = beatTracker.getLastOdf();
        if (frame.beat) {
            onsetCount++;
        }

        float detectedBpm = beatTracker.getBPM();
        if (frame.valid && detectedBpm >= cMinBpm && detectedBpm <= cMaxBpm) {
            frame.bpm = detectedBpm;
        } else {
            frame.bpm = 0.0f;
        }

        return frame;

    } // beginAudioFrame()

    //=====================================================================
    // Global audio frame cache
    // Keeps a single, per-loop snapshot for other programs to read.
    //=====================================================================

    AudioFrame gAudioFrame;
    bool gAudioFrameInitialized = false;
    uint32_t gAudioFrameLastMs = 0;

    inline const AudioFrame& updateAudioFrame(binConfig& b) {
        uint32_t now = fl::millis();

        if (gAudioFrameInitialized && now == gAudioFrameLastMs) {
            if (gAudioFrame.fft_norm_valid) {
                return gAudioFrame;
            }
        }

        const AudioFrame& frame = beginAudioFrame(b);
        gAudioFrame = frame;
        gAudioFrameInitialized = true;
        gAudioFrameLastMs = now;
        return gAudioFrame;
    }

    inline const AudioFrame& getAudioFrame() {
        return gAudioFrame;
    }

    //=========================================================================
    // Debug output
    //=========================================================================

    void printCalibrationDiagnostic() {
        const auto& f = gAudioFrame;
        FASTLED_DBG("BPM " << f.bpm);
        FASTLED_DBG("rmsNorm " << f.rms_norm);
        FASTLED_DBG("treNorm " << f.treble_norm);
        FASTLED_DBG("midNorm " << f.mid_norm);
        FASTLED_DBG("bassNorm " << f.bass_norm);
        /*FASTLED_DBG("--- ");
        FASTLED_DBG("rmsFact " << f.rms_factor);
        FASTLED_DBG("treFact " << f.treble_factor);
        FASTLED_DBG("midFact " << f.mid_factor);
        FASTLED_DBG("bassFact " << f.bass_factor);*/
        FASTLED_DBG("---------- ");
    }

    void printBeatTrackerDiagnostic() {
        const auto& f = gAudioFrame;
        FASTLED_DBG("beat " << (beatDetected ? 1 : 0)
                            << " bpm " << beatTracker.getBPM()
                            //<< " frame.bpm " << f.bpm
                            << " conf " << beatTracker.getConfidence());
        FASTLED_DBG("odf " << beatTracker.getLastOdf()
                           << " todf " << beatTracker.getLastTempoOdf()
                           << " thr " << beatTracker.getLastThreshold()
                           << " tempoMs " << beatTracker.getTempoMs()
                           << " tconf " << beatTracker.getTempoConfidence());
        FASTLED_DBG("---------- ");
    }

} // namespace myAudio
