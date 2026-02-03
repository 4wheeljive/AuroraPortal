#pragma once

#include "fl/audio.h"
#include "fl/fft.h"
#include "fl/audio/audio_context.h"
#include "fl/fx/audio/audio_processor.h"
#include "fl/math_macros.h"
#include "audioInput.h"
#include "bleControl.h"


namespace myAudio {

    using namespace fl;

    //=========================================================================
    // Audio filtering configuration
    // Applied at the source before AudioProcessor for clean beat detection,
    // FFT, and all downstream processing.
    //=========================================================================

    // Spike filtering: I2S occasionally produces spurious samples near int16_t max/min
    constexpr int16_t SPIKE_THRESHOLD = 10000;  // Samples beyond this are glitches

    // Noise gate: Signals below this RMS are considered silence
    // Prevents beat detection from triggering on background noise (fans, etc.)
    // Use higher threshold with hysteresis to prevent flickering at boundary
    //constexpr float NOISE_GATE_OPEN = 60.0f;   // Signal must exceed this to open gate
    //constexpr float NOISE_GATE_CLOSE = 30.0f;  // Signal must fall below this to close gate
    //constexpr float NOISE_GATE_OPEN = 0.0f;   // Calibration: effectively disable gate
    //constexpr float NOISE_GATE_CLOSE = 0.0f;  // Calibration: effectively disable gate
    
    //=========================================================================
    // Core audio objects
    //=========================================================================

    AudioSample currentSample;      // Raw sample from I2S (kept for diagnostics)
    AudioSample filteredSample;     // Spike-filtered sample for processing
    AudioProcessor audioProcessor;
    bool audioProcessingInitialized = false;

    // Buffer for filtered PCM data
    int16_t filteredPcmBuffer[512];  // Matches I2S_AUDIO_BUFFER_LEN

    //=========================================================================
    // State populated by callbacks (reactive approach)
    //=========================================================================

    // Beat/tempo state
    float currentBPM = 0.0f;
    uint32_t lastBeatTime = 0;
    uint32_t beatCount = 0;
    uint32_t onsetCount = 0;
    bool beatDetected = false;
    float lastOnsetStrength = 0.0f;

    // Energy levels from callbacks
    float bassLevel = 0.0f;
    float midLevel = 0.0f;
    float trebleLevel = 0.0f;
    float energyLevel = 0.0f;
    float peakLevel = 0.0f;

    //=========================================================================
    // FFT configuration
    //=========================================================================

    //constexpr uint8_t NUM_FFT_BINS = 16;
    constexpr uint8_t NUM_FFT_BINS = 32;
    constexpr float FFT_MIN_FREQ = 174.6f;   // ~G3
    constexpr float FFT_MAX_FREQ = 4698.3f;  // ~D8

    //=========================================================================
    // Visualization normalization & auto-calibration
    //=========================================================================

    struct AudioVizConfig {
        float noiseFloorFft = 0.00f;    // 0..1 normalized (applied after bins_db/100)
        float noiseFloorLevel = 0.00f;  // 0..1 normalized for RMS/peak
        float fftGain = 8.0f;           // User gain for FFT-based visuals
        float levelGain = 12.0f;        // User gain for RMS/peak visuals
        //float bandGain = 6.0f;          // User gain for bass/mid/treble visuals
        //float bandPeakDecay = 0.95f;    // Peak follower decay (0.0-1.0)
        bool autoGain = true;
        float autoGainTarget = 0.7f;
        bool autoNoiseFloor = false;
        float autoNoiseFloorAlpha = 0.01f;
        float autoNoiseFloorMin = 0.0f;
        float autoNoiseFloorMax = 0.5f;

        // BPM calibration: Beat detectors often double-count (kick+snare = 2 beats per beat)
        // Set to 0.5 if reported BPM is consistently ~2x actual tempo
        float bpmScaleFactor = 0.5f;    // 1.0 = use raw, 0.5 = halve the BPM
        float bpmMinValid = 60.0f;      // Ignore BPM below this
        float bpmMaxValid = 200.0f;     // Ignore BPM above this
    };

    AudioVizConfig vizConfig;
    float autoGainValue = 1.0f; 

    inline void syncVizConfigFromControls() {
        // Sync UI-controlled parameters into the audio visualization config.
        // These values are managed in bleControl.h.
        vizConfig.noiseFloorFft = cNoiseFloorFft;
        vizConfig.noiseFloorLevel = cNoiseFloorLevel;
        vizConfig.fftGain = cFftGain;
        vizConfig.levelGain = cLevelGain;
        vizConfig.autoGainTarget = cAutoGainTarget;
        vizConfig.autoNoiseFloorAlpha = cAutoNoiseFloorAlpha;
        vizConfig.autoNoiseFloorMin = cAutoNoiseFloorMin;
        vizConfig.autoNoiseFloorMax = cAutoNoiseFloorMax;
        vizConfig.autoGain = autoGain;
        vizConfig.autoNoiseFloor = autoNoiseFloor;
        vizConfig.bpmScaleFactor = cBpmScaleFactor;
    }

    void updateAutoGain(float level) {
        if (!vizConfig.autoGain) {
            autoGainValue = 1.0f;
            return;
        }

        static float avgLevel = 0.0f;
        avgLevel = avgLevel * 0.95f + level * 0.05f;

        if (avgLevel > 0.01f) {
            float gainAdjust = vizConfig.autoGainTarget / avgLevel;
            gainAdjust = fl::clamp(gainAdjust, 0.5f, 2.0f);
            autoGainValue = autoGainValue * 0.9f + gainAdjust * 0.1f;
        }
    }

    void updateAutoNoiseFloor(float level) {
        if (!vizConfig.autoNoiseFloor) {
            return;
        }

        // Only adapt when near the existing floor to avoid chasing loud signals.
        if (level < (vizConfig.noiseFloorLevel + 0.02f)) {
            float nf = vizConfig.noiseFloorLevel * (1.0f - vizConfig.autoNoiseFloorAlpha)
                       + level * vizConfig.autoNoiseFloorAlpha;
            vizConfig.noiseFloorLevel = fl::clamp(nf, vizConfig.autoNoiseFloorMin, vizConfig.autoNoiseFloorMax);
        }
    }

    //=========================================================================
    // Initialize audio processing with callbacks
    //=========================================================================

    // DIAGNOSTIC: Track callback invocations
    uint32_t callbackEnergyCount = 0;
    uint32_t callbackBassCount = 0;
    uint32_t callbackPeakCount = 0;

    void initAudioProcessing() {
        if (audioProcessingInitialized) {
            return;
        }

        // Beat detection callbacks
        audioProcessor.onBeat([]() {
            beatCount++;
            //lastBeatTime = fl::millis();
            lastBeatTime = currentSample.timestamp();
            beatDetected = true;
            // Serial.print("BEAT #");
            // Serial.println(beatCount);
        });

        audioProcessor.onOnset([](float strength) {
            onsetCount++;
            lastOnsetStrength = strength;
            // Serial.print("Onset strength=");
            // Serial.println(strength);
        });

        audioProcessor.onTempoChange([](float bpm, float confidence) {
            currentBPM = bpm;
            // Serial.print("Tempo: ");
            // Serial.print(bpm);
            // Serial.print(" BPM (confidence: ");
            // Serial.print(confidence);
            // Serial.println(")");
        });

        // Frequency band callbacks
        audioProcessor.onBass([](float level) {
            bassLevel = level;
            callbackBassCount++;
        });

        audioProcessor.onMid([](float level) {
            midLevel = level;
        });

        audioProcessor.onTreble([](float level) {
            trebleLevel = level;
        });

        // Energy callbacks
        audioProcessor.onEnergy([](float rms) {
            energyLevel = rms;
            callbackEnergyCount++;
        });

        audioProcessor.onPeak([](float peak) {
            peakLevel = peak;
            callbackPeakCount++;
        });

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
        static bool gateOpen = false;

        //if (blockRMS >= NOISE_GATE_OPEN) {
        if (blockRMS >= cNoiseGateOpen) {
            gateOpen = true;
        //} else if (blockRMS < NOISE_GATE_CLOSE) {
        } else if (blockRMS < cNoiseGateClose) {
            gateOpen = false;
        }
        // Between CLOSE and OPEN thresholds, gate maintains its previous state

        // If gate is closed, zero out entire buffer
        if (!gateOpen) {
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

    // ----------------------------------------------------------------

    // Get the AudioContext for direct FFT access
    fl::shared_ptr<AudioContext> getContext() {
        return audioProcessor.getContext();
    }
   
    // Get RMS from the filtered sample
    // Spike filtering and DC correction are now done at the source in sampleAudio()
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
            // Moderate attack: 50% new, 50% old
            smoothedRMS = median * 0.5f + smoothedRMS * 0.5f;
        } else {
            // Moderate decay: 40% new, 60% old
            smoothedRMS = median * 0.4f + smoothedRMS * 0.6f;
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

    // Unified FFT path (avoids AudioContext stack usage)
    // Uses static FFT storage and explicit args for consistent bins.
    static fl::FFTBins fftBins(myAudio::NUM_FFT_BINS);
    static fl::FFT fftEngine;

    const fl::FFTBins* getFFT() {
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
            NUM_FFT_BINS,
            FFT_MIN_FREQ,
            FFT_MAX_FREQ,
            sampleRate
        );

        fl::span<const fl::i16> span(pcm.data(), pcm.size());
        fftEngine.run(span, &fftBins, args);
        return &fftBins;
    }

    const fl::FFTBins* getFFT_direct() {
        return getFFT();
    }

    //=====================================================================
    // Per-frame snapshot helper
    // Ensures one audio sample -> one set of derived values per frame.
    //=====================================================================

    struct AudioFrame {
        bool valid = false;
        uint32_t timestamp = 0;
        bool beat = false;
        float rms_raw = 0.0f;
        float rms = 0.0f;
        float rms_norm = 0.0f;
        float rms_fast_norm = 0.0f;
        float bass_norm = 0.0f;
        float mid_norm = 0.0f;
        float treble_norm = 0.0f;
        float bass = 0.0f;
        float mid = 0.0f;
        float treble = 0.0f;
        float energy = 0.0f;
        float peak = 0.0f;
        float peak_norm = 0.0f;
        float bpm = 0.0f;
        float bpmScale = 1.0f;
        const fl::FFTBins* fft = nullptr;
        bool fft_norm_valid = false;
        float fft_norm[NUM_FFT_BINS] = {0};
        fl::Slice<const int16_t> pcm;
    };

    inline const AudioFrame& beginAudioFrame(bool computeFft = true) {
        static AudioFrame frame;
        static uint32_t lastFftTimestamp = 0;

        syncVizConfigFromControls();
        sampleAudio();

        frame.valid = filteredSample.isValid();
        frame.timestamp = currentSample.timestamp();
        frame.beat = beatDetected;
        frame.rms_raw = filteredSample.rms();
        frame.rms = getRMS();
        frame.bass = bassLevel;
        frame.mid = midLevel;
        frame.treble = trebleLevel;
        frame.energy = energyLevel;
        frame.peak = peakLevel;

        // Apply BPM calibration: scale and validate
        float scaledBpm = currentBPM * vizConfig.bpmScaleFactor;
        if (scaledBpm >= vizConfig.bpmMinValid && scaledBpm <= vizConfig.bpmMaxValid) {
            frame.bpm = scaledBpm;
            frame.bpmScale = vizConfig.bpmScaleFactor;
        } else if (currentBPM >= vizConfig.bpmMinValid && currentBPM <= vizConfig.bpmMaxValid) {
            // Raw BPM is valid but scaled isn't - use raw
            frame.bpm = currentBPM;
            frame.bpmScale = 1.0f;
        } else {
            // Neither is valid - keep previous or zero
            frame.bpm = 0.0f;
            frame.bpmScale = vizConfig.bpmScaleFactor;
        }

        frame.pcm = filteredSample.pcm();

        if (!frame.valid) {
            frame.fft = nullptr;
            frame.fft_norm_valid = false;
            frame.rms_norm = 0.0f;
            frame.peak_norm = 0.0f;
            frame.bass_norm = 0.0f;
            frame.mid_norm = 0.0f;
            frame.treble_norm = 0.0f;
            for (uint8_t i = 0; i < NUM_FFT_BINS; i++) {
                frame.fft_norm[i] = 0.0f;
            }
            return frame;
        }

        // Normalize RMS/peak and update auto-calibration
        float rmsNormRaw = frame.rms / 32768.0f;
        float rmsNormFast = frame.rms_raw / 32768.0f;
        float peakNormRaw = frame.peak / 32768.0f;
        rmsNormRaw = fl::clamp(rmsNormRaw, 0.0f, 1.0f);
        rmsNormFast = fl::clamp(rmsNormFast, 0.0f, 1.0f);
        peakNormRaw = fl::clamp(peakNormRaw, 0.0f, 1.0f);

        updateAutoNoiseFloor(rmsNormRaw);
        updateAutoGain(rmsNormRaw);

        //float gainApplied = vizConfig.gain * autoGainValue;
        float gainAppliedLevel = vizConfig.levelGain * autoGainValue;
        float gainAppliedFft = vizConfig.fftGain * autoGainValue;
        //float gainAppliedBand = vizConfig.bandGain * autoGainValue;

        frame.rms_norm = rmsNormRaw;
        frame.rms_fast_norm = rmsNormFast;
        frame.rms_norm = fl::clamp(FL_MAX(0.0f, frame.rms_norm - vizConfig.noiseFloorLevel) * gainAppliedLevel, 0.0f, 1.0f);
        frame.rms_fast_norm = fl::clamp(FL_MAX(0.0f, frame.rms_fast_norm - vizConfig.noiseFloorLevel) * gainAppliedLevel, 0.0f, 1.0f);

        frame.peak_norm = peakNormRaw;
        frame.peak_norm = fl::clamp(FL_MAX(0.0f, frame.peak_norm - vizConfig.noiseFloorLevel) * gainAppliedLevel, 0.0f, 1.0f);

        // AudioProcessor's band callbacks have aggressive AGC and are always near 1.0
        // regardless of actual audio level. Instead, derive bands from FFT bins which
        // respond correctly to actual signal levels.
        //
        // FFT config: 32 bins from ~175Hz to ~4700Hz (logarithmic spacing)
        // Bass: bins 0-7 (~175-400Hz), Mid: bins 8-19 (~400-1500Hz), Treble: bins 20-31 (~1500-4700Hz)

        // Compute FFT first, then derive bands from FFT bins
        if (computeFft && frame.timestamp != lastFftTimestamp) {
            frame.fft = getFFT();
            lastFftTimestamp = frame.timestamp;
            frame.fft_norm_valid = false;

            if (frame.fft && frame.fft->bins_db.size() > 0) {
                // First pass: compute pre-gain values for band calculation
                float preGainBins[NUM_FFT_BINS];
                for (uint8_t i = 0; i < NUM_FFT_BINS; i++) {
                    float mag = 0.0f;
                    if (i < frame.fft->bins_db.size()) {
                        mag = frame.fft->bins_db[i] / 100.0f;  // normalize dB bins
                    }
                    mag = FL_MAX(0.0f, mag - vizConfig.noiseFloorFft);
                    preGainBins[i] = fl::clamp(mag, 0.0f, 1.0f);

                    // Apply visual gain for spectrum displays
                    frame.fft_norm[i] = fl::clamp(mag * gainAppliedFft, 0.0f, 1.0f);
                }
                frame.fft_norm_valid = true;

                // Derive bass/mid/treble from PRE-GAIN FFT bins (32 bins total)
                // This ensures bands track actual signal level, not visual gain
                // Bass: bins 0-7, Mid: bins 8-19, Treble: bins 20-31
                float bassSum = 0.0f, midSum = 0.0f, trebleSum = 0.0f;
                for (uint8_t i = 0; i < 8; i++) bassSum += preGainBins[i];
                for (uint8_t i = 8; i < 20; i++) midSum += preGainBins[i];
                for (uint8_t i = 20; i < NUM_FFT_BINS; i++) trebleSum += preGainBins[i];

                // FFT bins_db/100 already produces usable 0-1 values (unlike RMS which needs 12x gain)
                // Scale bands relative to rmsNormRaw so they track with overall audio level
                // This makes bands represent relative frequency content at current volume
                float bassAvg = bassSum / 8.0f;
                float midAvg = midSum / 12.0f;
                float trebleAvg = trebleSum / 12.0f;

                // Use RMS-proportional scaling: bands scale with overall audio level
                // rmsNormRaw is pre-gain (0.001-0.02 typical), bin averages are post-dB-norm (0.4-0.8 typical)
                // Ratio adjusts bands to track with RMS-based visuals
                float rmsScale = rmsNormRaw * gainAppliedLevel;  // Same as final rms_norm (before noise floor)
                float binScale = (bassAvg + midAvg + trebleAvg) / 3.0f;  // Average FFT level
                float bandScaleFactor = (binScale > 0.01f) ? (rmsScale / binScale) : 1.0f;

                frame.bass_norm = fl::clamp(bassAvg * bandScaleFactor, 0.0f, 1.0f);
                frame.mid_norm = fl::clamp(midAvg * bandScaleFactor, 0.0f, 1.0f);
                frame.treble_norm = fl::clamp(trebleAvg * bandScaleFactor, 0.0f, 1.0f);

                // OLD: applied levelGain directly to bands (caused saturation at 1.0)
                // float bandGain = gainAppliedLevel;
                // frame.bass_norm = fl::clamp((bassSum / 8.0f) * bandGain, 0.0f, 1.0f);
                // frame.mid_norm = fl::clamp((midSum / 12.0f) * bandGain, 0.0f, 1.0f);
                // frame.treble_norm = fl::clamp((trebleSum / 12.0f) * bandGain, 0.0f, 1.0f);

                // OLD: used post-gain bins (caused bands to be pinned at 1.0)
                // float bassSum = 0.0f, midSum = 0.0f, trebleSum = 0.0f;
                // for (uint8_t i = 0; i < 8; i++) bassSum += frame.fft_norm[i];
                // for (uint8_t i = 8; i < 20; i++) midSum += frame.fft_norm[i];
                // for (uint8_t i = 20; i < NUM_FFT_BINS; i++) trebleSum += frame.fft_norm[i];
                // frame.bass_norm = fl::clamp(bassSum / 8.0f * 1.5f, 0.0f, 1.0f);
                // frame.mid_norm = fl::clamp(midSum / 12.0f * 1.5f, 0.0f, 1.0f);
                // frame.treble_norm = fl::clamp(trebleSum / 12.0f * 1.5f, 0.0f, 1.0f);
            } else {
                for (uint8_t i = 0; i < NUM_FFT_BINS; i++) {
                    frame.fft_norm[i] = 0.0f;
                }
                frame.bass_norm = 0.0f;
                frame.mid_norm = 0.0f;
                frame.treble_norm = 0.0f;
            }
        } else if (!computeFft) {
            frame.fft = nullptr;
            frame.fft_norm_valid = false;
            for (uint8_t i = 0; i < NUM_FFT_BINS; i++) {
                frame.fft_norm[i] = 0.0f;
            }
            // Without FFT, fall back to RMS-scaled approximation
            frame.bass_norm = frame.rms_norm;
            frame.mid_norm = frame.rms_norm;
            frame.treble_norm = frame.rms_norm;
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

    inline const AudioFrame& updateAudioFrame(bool computeFft = false) {
        uint32_t now = fl::millis();

        if (gAudioFrameInitialized && now == gAudioFrameLastMs) {
            if (!computeFft || gAudioFrame.fft_norm_valid) {
                return gAudioFrame;
            }
        }

        const AudioFrame& frame = beginAudioFrame(computeFft);
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

    void printAudioDebug() {
        EVERY_N_MILLISECONDS(250) {
            Serial.print("RMS: ");
            Serial.print(getRMS());
            Serial.print(" | Bass: ");
            Serial.print(bassLevel);
            Serial.print(" | Mid: ");
            Serial.print(midLevel);
            Serial.print(" | Treble: ");
            Serial.print(trebleLevel);

            const fl::FFTBins* fft = getFFT();
            if (fft && fft->bins_raw.size() > 0) {
                Serial.print(" | FFT[0]: ");
                Serial.print(fft->bins_raw[0]);
                Serial.print(" FFT[8]: ");
                Serial.print(fft->bins_raw[8]);
            }
            Serial.println();
        }
    }

    void printAudioBandsDebug() {
        EVERY_N_MILLISECONDS(1500) {
            const auto& frame = getAudioFrame();
            Serial.print("raw rms=");
            Serial.print(frame.rms);
            Serial.print(" bass=");
            Serial.print(frame.bass);
            Serial.print(" mid=");
            Serial.print(frame.mid);
            Serial.print(" treble=");
            Serial.print(frame.treble);

            Serial.print(" | norm rms=");
            Serial.print(frame.rms_norm, 3);
            Serial.print(" bass=");
            Serial.print(frame.bass_norm, 3);
            Serial.print(" mid=");
            Serial.print(frame.mid_norm, 3);
            Serial.print(" treble=");
            Serial.print(frame.treble_norm, 3);
            Serial.println();
        }
    }

    // Calibration diagnostic - shows FFT-derived band values
    void printCalibrationDiagnostic() {
        EVERY_N_MILLISECONDS(500) {
            const auto& f = gAudioFrame;
            Serial.print("[AUDIO] BPM:");
            Serial.print(f.bpm, 0);
            Serial.print(" rms=");
            Serial.print(f.rms_norm, 2);
            Serial.print(" | FFT bands: bass=");
            Serial.print(f.bass_norm, 2);
            Serial.print(" mid=");
            Serial.print(f.mid_norm, 2);
            Serial.print(" tre=");
            Serial.print(f.treble_norm, 2);
            Serial.println();
        }
    }

    void printRmsCalibration(const AudioFrame& frame) {
        EVERY_N_MILLISECONDS(1000) {
            Serial.print("[RMS] raw=");
            Serial.print(frame.rms_raw, 1);
            Serial.print(" smooth=");
            Serial.print(frame.rms, 1);
            Serial.print(" norm=");
            Serial.print(frame.rms_norm, 3);
            Serial.print(" fast_norm=");
            Serial.print(frame.rms_fast_norm, 3);
            Serial.println();
        }
    }

    //=========================================================================
    // Comprehensive audio diagnostic for calibration
    // Prints CSV-friendly output for analysis
    //=========================================================================

    void runAudioDiagnostic() {
        if (!currentSample.isValid()) {
            EVERY_N_MILLISECONDS(500) {
                Serial.println("[DIAG] No valid sample");
            }
            return;
        }

        auto pcm = currentSample.pcm();
        size_t n = pcm.size();
        if (n == 0) return;

        // Calculate statistics
        int64_t sum = 0;
        int16_t minVal = pcm[0], maxVal = pcm[0];

        // Count samples at extremes (saturation detection)
        uint16_t countAtMax = 0;  // samples >= 32000
        uint16_t countAtMin = 0;  // samples <= -32000
        uint16_t countNearZero = 0;  // samples between -100 and 100

        for (size_t i = 0; i < n; i++) {
            sum += pcm[i];
            if (pcm[i] < minVal) minVal = pcm[i];
            if (pcm[i] > maxVal) maxVal = pcm[i];

            // Saturation detection
            if (pcm[i] >= 32000) countAtMax++;
            if (pcm[i] <= -32000) countAtMin++;
            if (pcm[i] >= -100 && pcm[i] <= 100) countNearZero++;
        }
        int16_t dcOffset = static_cast<int16_t>(sum / n);

        // Use int32_t for peak-to-peak to avoid overflow
        int32_t peakToPeak = static_cast<int32_t>(maxVal) - static_cast<int32_t>(minVal);

        // Calculate DC-corrected RMS
        uint64_t sumSq = 0;
        for (size_t i = 0; i < n; i++) {
            int32_t corrected = static_cast<int32_t>(pcm[i]) - dcOffset;
            sumSq += corrected * corrected;
        }
        float rmsCorr = fl::sqrtf(static_cast<float>(sumSq) / n);

        // Also get the uncorrected RMS for comparison
        float rmsRaw = currentSample.rms();

        // Track saturation statistics over time
        static uint32_t totalSamples = 0;
        static uint32_t saturatedBlocks = 0;
        static uint32_t goodBlocks = 0;
        totalSamples++;

        bool isSaturated = (countAtMax > 0 || countAtMin > 0);
        if (isSaturated) {
            saturatedBlocks++;
        } else {
            goodBlocks++;
        }

        // Get the filtered RMS (what we'll actually use for visualizations)
        float rmsFiltered = getRMS();

        // Print header once
        static bool headerPrinted = false;
        if (!headerPrinted) {
            Serial.println();
            Serial.println("=== AUDIO DIAGNOSTIC ===");
            Serial.println("Spikes,RMS_Unfilt,RMS_Filt,DC,Min,Max,Status");
            headerPrinted = true;
        }

        // Print CSV data every 250ms
        EVERY_N_MILLISECONDS(250) {
            Serial.print(countAtMax + countAtMin);
            Serial.print(",");
            Serial.print(rmsCorr, 0);
            Serial.print(",");
            Serial.print(rmsFiltered, 0);
            Serial.print(",");
            Serial.print(dcOffset);
            Serial.print(",");
            Serial.print(minVal);
            Serial.print(",");
            Serial.print(maxVal);
            Serial.print(",");

            // Status indicator based on FILTERED RMS
            // Calibrated for INMP441 with spike filtering:
            //   Quiet room: 10-50, Speech/claps: 100-400, Loud: 400+
            if (rmsFiltered < 50) {
                Serial.print("quiet");
            } else if (rmsFiltered < 150) {
                Serial.print("low");
            } else if (rmsFiltered < 400) {
                Serial.print("medium");
            } else {
                Serial.print("LOUD");
            }

            // Note if spikes were filtered
            if (countAtMax > 0 || countAtMin > 0) {
                Serial.print(" (");
                Serial.print(countAtMax + countAtMin);
                Serial.print(" spikes filtered)");
            }
            Serial.println();
        }

        // Print summary statistics periodically
        EVERY_N_SECONDS(10) {
            float spikePercent = (totalSamples > 0) ? (saturatedBlocks * 100.0f) / totalSamples : 0;
            Serial.println();
            Serial.print("--- Stats: ");
            Serial.print(goodBlocks);
            Serial.print(" clean blocks, ");
            Serial.print(saturatedBlocks);
            Serial.print(" with spikes (");
            Serial.print(spikePercent, 0);
            Serial.print("%) - spikes filtered OK ---");
            Serial.println();
        }
    } // runAudioDiagnostic()

} // namespace myAudio
