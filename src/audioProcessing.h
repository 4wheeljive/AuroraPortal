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

    // Custom beat detector (testing in lieu of FastLED's BPM detection)
    BeatDetector beatDetector;

    // Buffer for filtered PCM data
    int16_t filteredPcmBuffer[512];  // Matches I2S_AUDIO_BUFFER_LEN

    //=========================================================================
    // State populated by callbacks (reactive approach)
    //=========================================================================

    // Beat/tempo state
    float currentBPM = 0.0f;
    bool beatDetected = false;
    uint32_t lastBeatTime = 0;
    uint32_t beatCount = 0;
    uint32_t onsetCount = 0;
    float lastOnsetStrength = 0.0f;

    /*
    // Energy levels from callbacks
    float bassLevel = 0.0f;
    float midLevel = 0.0f;
    float trebleLevel = 0.0f;
    float energyLevel = 0.0f;
    float peakLevel = 0.0f;
    */
    //=========================================================================
    // FFT configuration
    //=========================================================================

    //bool maxBins = true;

    // Maximum FFT bins (compile-time constant for array sizing)
    constexpr uint8_t MAX_FFT_BINS = 32;

    constexpr float FFT_MIN_FREQ = 20.0f;    
    constexpr float FFT_MAX_FREQ = 16000.f;

    enum BinConfig {
        BIN16 = 0,
        BIN32   
    };	

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
        float autoGainTarget = 0.7f;
        bool autoFloor = false;
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

        static float avgLevel = 0.0f;
        avgLevel = avgLevel * 0.95f + level * 0.05f;

        if (avgLevel > 0.01f) {
            float gainAdjust = vizConfig.autoGainTarget / avgLevel;
            gainAdjust = fl::clamp(gainAdjust, 0.5f, 2.0f);
            autoGainValue = autoGainValue * 0.9f + gainAdjust * 0.1f;
        }
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

        // Beat detection callbacks
        audioProcessor.onBeat([]() {
            beatCount++;
            //lastBeatTime = fl::millis();
            lastBeatTime = currentSample.timestamp();
            beatDetected = true;
            // Serial.print("BEAT #");
            // Serial.println(beatCount);
        });

        audioProcessor.onTempoChange([](float bpm, float confidence) {
            currentBPM = bpm;
            // Serial.print("Tempo: ");
            // Serial.print(bpm);
            // Serial.print(" BPM (confidence: ");
            // Serial.print(confidence);
            // Serial.println(")");
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
        float rms_fast_norm = 0.0f;
        float bass_norm = 0.0f;
        float mid_norm = 0.0f;
        float treble_norm = 0.0f;
        //float bass = 0.0f;
        //float mid = 0.0f;
        //float treble = 0.0f;
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
        frame.rms_raw = filteredSample.rms();
        frame.rms = getRMS();

        // Custom beat detection using bass energy from FFT
        // Compute FFT early to get bass energy for beat detector
        const fl::FFTBins* fftForBeat = getFFT(b);
        float bassEnergy = 0.0f;

        if (fftForBeat && fftForBeat->bins_db.size() >= 4) {
            // Sum bass frequency bins and normalize
            for (uint8_t i = 0; i < b.bassBins; i++) {
                float mag = fftForBeat->bins_db[i] / 100.0f;
                mag = FL_MAX(0.0f, mag - vizConfig.audioFloorFft);
                bassEnergy += fl::clamp(mag, 0.0f, 1.0f);
            }
            bassEnergy /= b.fBassBins;  // Average
        }

        // Update beat detector with bass energy
        beatDetector.update(bassEnergy, frame.timestamp);
        frame.beat = beatDetector.isBeat();

        // Get BPM from custom detector (no scale factor needed - it measures actual tempo)
        float detectedBpm = beatDetector.getBPM();
        if (detectedBpm >= vizConfig.bpmMinValid && detectedBpm <= vizConfig.bpmMaxValid) {
            frame.bpm = detectedBpm;
        } else {
            frame.bpm = 0.0f;
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
            for (uint8_t i = 0; i < b.NUM_FFT_BINS; i++) {
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

        updateAutoFloor(rmsNormRaw);
        updateAutoGain(rmsNormRaw);

        float gainAppliedLevel = vizConfig.gainLevel * autoGainValue;
        float gainAppliedFft = vizConfig.gainFft * autoGainValue;
        
        frame.rms_norm = rmsNormRaw;
        frame.rms_fast_norm = rmsNormFast;
        frame.rms_norm = fl::clamp(FL_MAX(0.0f, frame.rms_norm - vizConfig.audioFloorLevel) * gainAppliedLevel, 0.0f, 1.0f);
        frame.rms_fast_norm = fl::clamp(FL_MAX(0.0f, frame.rms_fast_norm - vizConfig.audioFloorLevel) * gainAppliedLevel, 0.0f, 1.0f);

        frame.peak_norm = peakNormRaw;
        frame.peak_norm = fl::clamp(FL_MAX(0.0f, frame.peak_norm - vizConfig.audioFloorLevel) * gainAppliedLevel, 0.0f, 1.0f);

        // Derive bands from FFT bins (band boundaries set in binConfig)
        // Note: FFT may already be computed by beat detection above
        if (frame.timestamp != lastFftTimestamp) {
            frame.fft = fftForBeat ? fftForBeat : getFFT(b);  // Reuse if already computed
            lastFftTimestamp = frame.timestamp;
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

                // Use RMS-proportional scaling: bands scale with overall audio level
                // rmsNormRaw is pre-gain (0.001-0.02 typical), bin averages are post-dB-norm (0.4-0.8 typical)
                // Ratio adjusts bands to track with RMS-based visuals
                float rmsScale = rmsNormRaw * gainAppliedLevel;  // Same as final rms_norm (before noise floor)
                float binScale = (bassAvg + midAvg + trebleAvg) / 3.0f;  // Average FFT level
                float bandScaleFactor = (binScale > 0.01f) ? (rmsScale / binScale) : 1.0f;

                frame.bass_norm = fl::clamp(bassAvg * bandScaleFactor, 0.0f, 1.0f);
                frame.mid_norm = fl::clamp(midAvg * bandScaleFactor, 0.0f, 1.0f);
                frame.treble_norm = fl::clamp(trebleAvg * bandScaleFactor, 0.0f, 1.0f);

            } else {
                for (uint8_t i = 0; i < b.NUM_FFT_BINS; i++) {
                    frame.fft_norm[i] = 0.0f;
                }
                frame.bass_norm = 0.0f;
                frame.mid_norm = 0.0f;
                frame.treble_norm = 0.0f;
            }
        } else {
            frame.fft = nullptr;
            frame.fft_norm_valid = false;
            for (uint8_t i = 0; i < b.NUM_FFT_BINS; i++) {
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

} // namespace myAudio