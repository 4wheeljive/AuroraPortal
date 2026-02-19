#pragma once

// =====================================================
// THIS IS EARLY-DEVELOPMENT EXPERIMENTAL STUFF
// USE AT YOUR OWN RISK! ;-)
// =====================================================

#include "fl/audio.h"
#include "fl/fft.h"
//#include "fl/audio/audio_context.h"
//#include "fl/fx/audio/audio_processor.h"
#include "fl/math_macros.h"
#include "audioInput.h"
#include "bleControl.h"

namespace myAudio {

    using namespace fl;

    //=========================================================================
    // Core audio objects
    //=========================================================================

    AudioSample currentSample;      // Raw sample from I2S (kept for diagnostics)
    AudioSample filteredSample;     // Spike-filtered sample for processing
    //AudioProcessor audioProcessor;
    bool audioProcessingInitialized = false;

    // Buffer for filtered PCM data
    int16_t filteredPcmBuffer[512];  // Matches I2S_AUDIO_BUFFER_LEN

    //=========================================================================
    // State populated by callbacks (reactive approach)
    //=========================================================================

    // Detector variables
    bool noiseGateOpen = false;
    float lastBlockRms = 0.0f;
    float lastAutoGainCeil = 0.0f;
    float lastAutoGainDesired = 0.0f;
    uint16_t lastValidSamples = 0;
    uint16_t lastClampedSamples = 0;
    int16_t lastPcmMin = 0;
    int16_t lastPcmMax = 0;

    //=========================================================================
    // FFT configuration
    // binConfig enables runtime switching of bin count/resolution
    //=========================================================================

    // Maximum FFT bins (compile-time constant for array sizing)
    constexpr uint8_t MAX_FFT_BINS = 32;

    constexpr float FFT_MIN_FREQ = 60.0f;    
    constexpr float FFT_MAX_FREQ = 16000.f;
 
    struct binConfig {
        //BinConfig binSize;
        uint8_t NUM_FFT_BINS;
        //uint8_t firstMidBin;
        //uint8_t firstTrebleBin;
        //uint8_t bassBins;
        //uint8_t midBins;
        //uint8_t trebleBins;
        //float fBassBins; // for use as divisor in averaging calculations
        //float fMidBins;
        //float fTrebleBins; 
    };

    binConfig bin16;
    binConfig bin32;

    void setBinConfig() {
        
        // 16-bin: log spacing 60–16000 Hz (need to recalculate ranges described below)
        // Bass: bins 0–5   (20–245 Hz)    6 bins
        // Mid:  bins 6–10  (245–1976 Hz)   5 bins
        // Tre:  bins 11–15 (1976–16000 Hz) 5 bins
        bin16.NUM_FFT_BINS = 16;
        /*
        bin16.firstMidBin = 6;
        bin16.firstTrebleBin = 11;
        bin16.bassBins = bin16.firstMidBin;
        bin16.midBins = bin16.firstTrebleBin - bin16.firstMidBin;
        bin16.trebleBins = bin16.NUM_FFT_BINS - bin16.firstTrebleBin;
        bin16.fBassBins = float(bin16.bassBins);
        bin16.fMidBins = float(bin16.midBins);
        bin16.fTrebleBins = float(bin16.trebleBins);
        */

        // 32-bin: log spacing 60–16000 Hz (need to recalculate ranges described below)
        // Bass: bins 0–11  (20–245 Hz)    12 bins
        // Mid:  bins 12–21 (245–1979 Hz)  10 bins
        // Tre:  bins 22–31 (1979–16000 Hz) 10 bins
        bin32.NUM_FFT_BINS = 32;
        /*
        bin32.firstMidBin = 12;
        bin32.firstTrebleBin = 22;
        bin32.bassBins = bin32.firstMidBin;
        bin32.midBins = bin32.firstTrebleBin - bin32.firstMidBin;
        bin32.trebleBins = bin32.NUM_FFT_BINS - bin32.firstTrebleBin;
        bin32.fBassBins = float(bin32.bassBins);
        bin32.fMidBins = float(bin32.midBins);
        bin32.fTrebleBins = float(bin32.trebleBins);
        */
    }

    //===========================================================================================
    // Visualization normalization & auto-calibration
    //===========================================================================================

    // Scale factors: single user control → domain-specific internal values
    // Level (RMS) values are tiny (~0.001-0.02), FFT/dB values are larger (~0.1-0.8)
    constexpr float FLOOR_SCALE_LEVEL = 0.05f;
    constexpr float FLOOR_SCALE_FFT   = 0.3f;
    constexpr float GAIN_SCALE_LEVEL  = 12.0f;
    constexpr float GAIN_SCALE_FFT    = 8.0f;

    // AudioVizConfig provides parameters for fine-tuning auto-gain/scaling/normalization of audio sample data 
    // for visualizer use  

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

        //float bpmScaleFactor = 0.5f;
        //float bpmMinValid = 50.0f;
        //float bpmMaxValid = 250.0f;
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
        //vizConfig.bpmScaleFactor  = cBpmScaleFactor;
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
        static bool prevGateOpen = false;

        // On gate-open transition: reset to clean defaults.
        // The smoothed RMS is still near zero at this point, so we
        // can't seed from the current level — use known-good values.
        if (noiseGateOpen && !prevGateOpen) {
            ceilingEstimate = 0.02f;
            autoGainValue = 1.0f;
        }
        prevGateOpen = noiseGateOpen;

        // Freeze auto-gain when noise gate is closed.
        // Prevents ceiling decay and gain buildup during silence.
        if (!noiseGateOpen) return;

        constexpr float targetPercentile = 0.90f;
        constexpr float alpha = 0.12f;  // upward effective: 0.012, ~80 frames ≈ 1.3 sec

        // Asymmetric proportional update: converges to the target quantile
        if (level > ceilingEstimate) {
            ceilingEstimate += alpha * (1.0f - targetPercentile) * (level - ceilingEstimate);
        } else {
            ceilingEstimate += alpha * targetPercentile * (level - ceilingEstimate);
        }
        ceilingEstimate = FL_MAX(ceilingEstimate, 0.0005f);  // prevent collapse
        lastAutoGainCeil = ceilingEstimate;

        // Solve for autoGainValue:
        //   rms_norm ≈ ceilingEstimate × gainLevel × autoGainValue = autoGainTarget
        float desired = vizConfig.autoGainTarget / (ceilingEstimate * vizConfig.gainLevel);
        desired = fl::clamp(desired, 0.1f, 8.0f);
        lastAutoGainDesired = desired;

        // Smooth the transition to avoid abrupt gain jumps
        autoGainValue = autoGainValue * 0.80f + desired * 0.20f;
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
    // Bin and Bus configuration
    //=========================================================================

    struct Bus;

    struct Bin {
        // Single-bus routing; nullptr = unassigned
        Bus* bus = nullptr;
    };

    Bin bin[MAX_FFT_BINS];

    static constexpr uint8_t NUM_BUSES = 3;

    struct Bus {
        uint8_t id = 0;
        // INPUTS
        float threshold = 0.40f;
        float minBeatInterval = 250.f; //milliseconds
        float expDecayFactor = 0.95f; // for exponential decay
        float rampAttack = 0.0f;
        float rampDecay = 300.f;
        float peakBase = 1.0f;

        // INTERNAL
        bool isActive = false;
        float avgLevel = 0.01f;  // linear scale: fft_pre = bins_raw/32768; typical music ~0.01-0.10
        float energyEMA = 0.0f;
        float relativeIncrease = 0.0f;
        uint32_t lastBeat = 0;
        float preNorm = 0.0f;
        float _norm = 0.0f;
        float _factor = 0.0f;

        // OUTPUTS
        bool newBeat = false;
        float beatBrightness = 0.0f;

    };

    Bus busA{.id = 0};
    Bus busB{.id = 1};
    Bus busC{.id = 2};

    void initBus(Bus& bus) {
        bus.newBeat = false;
        bus.isActive = false;
        bus.threshold = 0.40f;
        bus.minBeatInterval = 250.f;
        bus.expDecayFactor = 0.85f;
        bus.avgLevel = 0.01f;
        bus.energyEMA = 0.0f;
        bus.lastBeat = 0;
        bus.preNorm = 0.0f;
        bus._norm = 0.0f;
        bus._factor = 0.0f;
        bus.beatBrightness = 0.0f;
        bus.rampAttack = 0.f;
        bus.rampDecay = 300.f;
        bus.peakBase = 1.0f;

    }

    void initBins() {
        for (uint8_t i = 0; i < MAX_FFT_BINS; i++ ) {
            bin[i].bus = nullptr; 
        }

        //bin[0].bus = &busA;
        bin[1].bus = &busA;
        bin[2].bus = &busA;
        bin[3].bus = &busA;
        //bin[4].bus = &busA;

        bin[7].bus = &busB;
        bin[8].bus = &busB;
        bin[9].bus = &busB;
        
        //bin[11].bus = &busC;
        bin[12].bus = &busC;
        bin[13].bus = &busC;

    }

    //=========================================================================
    // Initialize audio processing with callbacks
    //=========================================================================

    void initAudioProcessing() {

        if (audioProcessingInitialized) { return; }

        // Initialize bin/bus configurations
        setBinConfig();
        initBus(busA);
        initBus(busB);
        initBus(busC);
        initBins();

        Serial.println("AudioProcessor initialized with callbacks");
        audioProcessingInitialized = true;
    }

    //=========================================================================
    // Sample audio and process
    //=========================================================================

    void sampleAudio() {

        if (!audioSource) {
            currentSample = AudioSample();
            filteredSample = AudioSample();
            return;
        }

        checkAudioInput();

        fl::string errorMsg;
        if (audioSource->error(&errorMsg)) {
            Serial.print("Audio error: ");
            Serial.println(errorMsg.c_str());
            currentSample = AudioSample();
            filteredSample = AudioSample();
            return;
        }

        // ACTUAL AUDIO SAMPLE CAPTURE
        // Drain all available audio buffers and keep only the newest sample.
        // This prevents accumulating stale audio when render FPS is slower
        // than the audio production rate.
        fl::vector_inlined<AudioSample, 16> samples;
        samples.clear();
        size_t readCount = audioSource->readAll(&samples);
        if (readCount == 0) {
            currentSample = AudioSample();
            filteredSample = AudioSample();
            return;
        }
        currentSample = samples[readCount - 1];

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

        int16_t pcmMin = 32767;
        int16_t pcmMax = -32768;
        for (size_t i = 0; i < nClamped; i++) {
            int16_t v = rawPcm[i];
            if (v < pcmMin) pcmMin = v;
            if (v > pcmMax) pcmMax = v;
        }
        if (nClamped == 0) {
            pcmMin = 0;
            pcmMax = 0;
        }
        lastPcmMin = pcmMin;
        lastPcmMax = pcmMax;
       
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
        lastBlockRms = blockRMS;
        lastValidSamples = static_cast<uint16_t>(validSamples);
        lastClampedSamples = static_cast<uint16_t>(nClamped);

        // EMA-smooth blockRMS before gate decision so brief noise spikes don't open the gate.
        // A single spike to 70 barely moves the EMA; sustained music builds it up within ~10 frames.
        static float blockRmsEMA = 0.0f;
        constexpr float rmsEmaAlpha = 0.15f;
        blockRmsEMA = blockRmsEMA * (1.0f - rmsEmaAlpha) + blockRMS * rmsEmaAlpha;
        lastBlockRms = blockRmsEMA;  // update diagnostic to show smoothed value

        // NOISE GATE with hysteresis to prevent flickering
        // Gate opens when smoothed signal exceeds cNoiseGateOpen
        // Gate closes when smoothed signal falls below cNoiseGateClose
        if (blockRmsEMA >= cNoiseGateOpen) {
            noiseGateOpen = true;
        } else if (blockRmsEMA < cNoiseGateClose) {
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
           // - only needed if there are active callbacks to trigger
        //audioProcessor.update(filteredSample);  // filtered
    
    } // sampleAudio()

    //===============================================================================

    // Get the AudioContext (if using) 
    // fl::shared_ptr<AudioContext> getContext() {
    //    return audioProcessor.getContext();
    //}
   
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

    // =====================================================================
    // FFT - Unified FFT path (avoids AudioContext stack usage)
    // Uses static FFT storage and explicit args for consistent bins.
    // =====================================================================

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
    // AUDIO FRAME - Per-frame snapshot helper
    // Ensures one audio sample -> one set of derived values per frame.
    //=====================================================================

    struct AudioFrame {
        bool valid = false;
        uint32_t timestamp = 0;
        //bool beat = false;
        //float bpm = 0.0f;
        //float bpmIndex = 1.0f;
        float rms_raw = 0.0f;
        float rms = 0.0f;
        float rms_norm = 0.0f;
        float rms_factor = 0.0f;
        float rms_fast_norm = 0.0f;
        float energy = 0.0f;
        float peak = 0.0f;
        float peak_norm = 0.0f;
        const fl::FFTBins* fft = nullptr;
        bool fft_norm_valid = false;
        float fft_pre[MAX_FFT_BINS] = {0};
        float fft_norm[MAX_FFT_BINS] = {0};
        fl::Slice<const int16_t> pcm;
        Bus busA;
        Bus busB;
        Bus busC;
    };

    // ---------------------------------------

    void updateBus(const AudioFrame& frame, const binConfig& b, Bus& bus) {
        bus.isActive = false;
        bus.newBeat = false;

        if (!frame.valid || !frame.fft_norm_valid) {
            bus._norm = 0.0f;
            bus._factor = 0.0f;
            return;
        }

        float sum = 0.0f;
        uint8_t count = 0;
        for (uint8_t i = 0; i < b.NUM_FFT_BINS; i++) {
            if (bin[i].bus == &bus) {
                sum += frame.fft_pre[i];
                count++;
            }
        }

        if (count > 0) {(bus.isActive = true);} 

        float avg = (count > 0) ? (sum / static_cast<float>(count)) : 0.0f;

        constexpr float eqAlpha = 0.02f;  // ~1-2 sec half-life
        bus.avgLevel += eqAlpha * (avg - bus.avgLevel);
        bus.avgLevel = FL_MAX(bus.avgLevel, 0.001f);  // linear scale floor

        // Store spectrally-flattened value (cross-cal and gain applied later)
        bus._norm = avg / bus.avgLevel;
    }

    void finalizeBus(const AudioFrame& frame, Bus& bus, float crossCalRatio, float gainApplied) {
        // Capture pre-finalize _norm (spectrally-flattened, before cross-cal/gain)
        bus.preNorm = bus._norm;

        if (!bus.isActive) return;

        // Absolute energy guard: suppress beat detection when the raw bin energy
        // is negligible. rawAvg recovers the actual mean fft_pre across this bus's
        // bins before whitening: rawAvg = (avg/avgLevel) * avgLevel = avg.
        // This prevents weak harmonic bleed from triggering a bus whose primary
        // frequency range isn't active. Tune minRawEnergy as needed.
        // fft_pre is now bins_raw/32768 (linear), so typical music signals
        // are ~0.01-0.10; harmonic bleed on quiet bins is <0.002.
        float rawAvg = bus.preNorm * bus.avgLevel;
        constexpr float minRawEnergy = 0.002f;

        // --- Beat detection on pre-finalize _norm so onset shape isn't distorted ---
        // Compare current energy against EMA baseline (check BEFORE updating
        // EMA so the onset spike isn't yet blended into the baseline).
        // Skip beat detection until EMA has warmed up: avoids spurious beats at
        // startup and after silence (where avgLevel decays slower than energyEMA,
        // causing preNorm to recover to ~1.0 while EMA is still near zero).
        constexpr float emaAlpha = 0.15f;   // ~6-7 frame half-life
        constexpr float emaWarmupFloor = 0.05f;
        if (bus.energyEMA >= emaWarmupFloor && rawAvg >= minRawEnergy) {
            float increase = bus.preNorm - bus.energyEMA;
            bus.relativeIncrease = increase / bus.energyEMA;

            uint32_t now = frame.timestamp;
            if (bus.relativeIncrease > bus.threshold && (now - bus.lastBeat) > bus.minBeatInterval) {
                bus.newBeat = true;
                bus.lastBeat = now;
            }
        } else {
            bus.relativeIncrease = 0.0f;
        }

        // Update EMA after beat check (always runs so baseline tracks signal)
        bus.energyEMA += emaAlpha * (bus.preNorm - bus.energyEMA);

        // --- Apply cross-cal and gain for visualization ---
        bus._norm = fl::clamp(bus._norm * crossCalRatio * gainApplied, 0.0f, 1.0f);

        constexpr float gamma = 0.5754f; // ln(0.5)/ln(0.3)
        bus._factor = 2.0f * fl::powf(bus._norm, gamma);
    }

    inline const AudioFrame& captureAudioFrame(binConfig& b) {
        static AudioFrame frame;
        static uint32_t lastFftTimestamp = 0;

        // *** STAGE: set current AudioVizConfig parameters  
        updateVizConfig();
        
        // *** STAGE: capture filtered audio sample
        sampleAudio();

        // Gate-open transition: reset per-bus EMA state so that avgLevel (alpha=0.02,
        // very slow) doesn't produce inflated _norm on the first beats after silence.
        // Without this, avgLevel decays to ~0.01 during gate-closed silence, causing
        // avg/avgLevel to spike to 10-40x for all buses equally when music resumes.
        {
            static bool prevGateForBus = false;
            if (noiseGateOpen && !prevGateForBus) {
                const float kResetLevel = 0.01f;  // linear FFT scale; matches Bus::avgLevel default
                busA.avgLevel = kResetLevel;  busA.energyEMA = 0.0f;
                busB.avgLevel = kResetLevel;  busB.energyEMA = 0.0f;
                busC.avgLevel = kResetLevel;  busC.energyEMA = 0.0f;
            }
            prevGateForBus = noiseGateOpen;
        }

        frame.valid = filteredSample.isValid();
        frame.timestamp = currentSample.timestamp();
        
        frame.pcm = filteredSample.pcm();

        // *** STAGE: Run FFT engine once per timestamp
        static const fl::FFTBins* lastFft = nullptr;
        const fl::FFTBins* fftForBeat = nullptr;
        float rmsNormFast = 0.0f;
        float timeEnergy = 0.0f;
        float beatBins[MAX_FFT_BINS] = {0.0f};
        bool beatBinsValid = false;
        float rmsPostFloor = 0.0f;
        float gainAppliedLevel = 1.0f;
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
        
        // *** STAGE: Get frame RMS and calculate _norm and _factor values
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
            rmsPostFloor = FL_MAX(0.0f, rmsNormRaw - vizConfig.audioFloorLevel);

            timeEnergy = FL_MAX(0.0f, rmsNormFast - vizConfig.audioFloorLevel);

            gainAppliedLevel = vizConfig.gainLevel * autoGainValue;
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
           
           
            // *** STAGE: Derive busses/bands from FFT bins (band boundaries set in binConfig),
            //            calculate _norm and _factor values
            frame.fft_norm_valid = false;
            if (frame.fft && frame.fft->bins_db.size() > 0) {
                for (uint8_t i = 0; i < b.NUM_FFT_BINS; i++) {
                    // --- Visualization path: dB-linear scale (perceptually uniform for display) ---
                    float mag_db = 0.0f;
                    if (i < frame.fft->bins_db.size()) {
                        mag_db = frame.fft->bins_db[i] / 100.0f;
                    }
                    mag_db = FL_MAX(0.0f, mag_db - vizConfig.audioFloorFft);
                    frame.fft_norm[i] = fl::clamp(mag_db * gainAppliedFft, 0.0f, 1.0f);

                    // --- Bus beat detection path: true linear amplitude ---
                    // bins_raw is the Q15 linear magnitude; /32768 normalizes to [0, ~1].
                    // A harmonic 30 dB below its fundamental is ~3% of it here,
                    // vs ~30% in the dB-linear (/100) domain — far better harmonic
                    // rejection for per-bus frequency discrimination.
                    float mag_lin = 0.0f;
                    if (i < frame.fft->bins_raw.size()) {
                        mag_lin = frame.fft->bins_raw[i] / 32768.0f;
                    }
                    frame.fft_pre[i] = fl::clamp(mag_lin, 0.0f, 1.0f);
                }
                for (uint8_t i = b.NUM_FFT_BINS; i < MAX_FFT_BINS; i++) {
                    frame.fft_pre[i] = 0.0f;
                    frame.fft_norm[i] = 0.0f;
                }
                frame.fft_norm_valid = true;

            } else { // if no valid fft data
          
                for (uint8_t i = 0; i < b.NUM_FFT_BINS; i++) {
                    frame.fft_pre[i] = 0.0f;
                    frame.fft_norm[i] = 0.0f;
                }
                frame.fft_norm_valid = false;
                // Without FFT, fall back to RMS-scaled approximation

                //TODO: Is something needed here reflecting the current program framework   
                /*
                OLD frame.bass_norm = frame.
                ...
                */
            }
        
        } else { // if frame not valid
     
            frame.fft = nullptr;
            frame.fft_norm_valid = false;
            frame.rms_norm = 0.0f;
            frame.peak_norm = 0.0f;
            /* OLD
            frame.bass_norm = 0.0f;
            frame.bass_factor = 0.0f;
            frame.mid_norm = 0.0f;
            frame.mid_factor = 0.0f;
            frame.treble_norm = 0.0f;
            frame.treble_factor = 0.0f;
            */
            for (uint8_t i = 0; i < b.NUM_FFT_BINS; i++) {
                frame.fft_pre[i] = 0.0f;
                frame.fft_norm[i] = 0.0f;
            }
      
        }

        // Update bus outputs (phase 1: compute spectrally-flattened values)
        updateBus(frame, b, busA);
        updateBus(frame, b, busB);
        updateBus(frame, b, busC);

        // Phase 2: Apply RMS-domain scaling and gain for visualization.
        // In steady state, whitened _norm ≈ 1.0, so bus._norm ≈ rmsPostFloor * gain ≈ rms_norm.
        // Using rmsPostFloor directly (vs. the previous slow-adapting EMA ratio) is equivalent
        // since the whitened avgFlat ≈ 1.0 anyway — and this avoids the EMA warm-up lag.
        {
            finalizeBus(frame, busA, rmsPostFloor, gainAppliedLevel);
            finalizeBus(frame, busB, rmsPostFloor, gainAppliedLevel);
            finalizeBus(frame, busC, rmsPostFloor, gainAppliedLevel);
        }

        frame.busA = busA;
        frame.busB = busB;
        frame.busC = busC;
       
        // *** STAGE: Return current AudioFrame frame to calling functions 
        // e.g., getAudio() --> updateAudioFrame() --> captureAudioFrame()  
        return frame;

    } // captureAudioFrame()

    //=====================================================================
    // Global audio frame cache
    // Keeps a single, per-loop snapshot for other programs to read.
    //=====================================================================

    AudioFrame gAudioFrame;
    bool gAudioFrameInitialized = false;
    uint32_t gAudioFrameLastMs = 0;
    bool audioLatencyDiagnostics = false;

    inline uint32_t getAudioSampleRate() {
        uint32_t sampleRate = fl::FFT_Args::DefaultSampleRate();
        if (config.is<fl::AudioConfigI2S>()) {
            sampleRate = static_cast<uint32_t>(config.get<fl::AudioConfigI2S>().mSampleRate);
        } else if (config.is<fl::AudioConfigPdm>()) {
            sampleRate = static_cast<uint32_t>(config.get<fl::AudioConfigPdm>().mSampleRate);
        }
        return sampleRate;
    }

    inline const AudioFrame& updateAudioFrame(binConfig& b) {
        uint32_t now = fl::millis();

        if (gAudioFrameInitialized && now == gAudioFrameLastMs) {
            if (gAudioFrame.fft_norm_valid) {
                return gAudioFrame;
            }
        }

        const AudioFrame& frame = captureAudioFrame(b);
        gAudioFrame = frame;
        gAudioFrameInitialized = true;
        gAudioFrameLastMs = now;

        if (audioLatencyDiagnostics) {
            struct LatencyStats {
                bool epochSet = false;
                int32_t epochOffsetMs = 0;
                uint32_t windowStartMs = 0;
                uint32_t lastFrameMs = 0;
                uint32_t frameCount = 0;
                uint64_t sumFrameMs = 0;
                uint32_t minFrameMs = 0xFFFFFFFFu;
                uint32_t maxFrameMs = 0;

                uint32_t sampleCount = 0;
                int32_t minLatencyMs = 0x7FFFFFFF;
                int32_t maxLatencyMs = -0x7FFFFFFF;
                int64_t sumLatencyMs = 0;

                uint32_t lastPcmSamples = 0;
                uint32_t lastSampleRate = 0;
                uint32_t invalidCount = 0;
            };

            static LatencyStats stats;

            if (stats.windowStartMs == 0) {
                stats.windowStartMs = now;
            }

            if (!frame.valid) {
                stats.invalidCount++;
            } else {
                if (!stats.epochSet) {
                    stats.epochOffsetMs = static_cast<int32_t>(now) - static_cast<int32_t>(frame.timestamp);
                    stats.epochSet = true;
                }
                int32_t alignedTimestampMs =
                    static_cast<int32_t>(frame.timestamp) + stats.epochOffsetMs;
                int32_t latencyMs = static_cast<int32_t>(now) - alignedTimestampMs;

                stats.sampleCount++;
                stats.sumLatencyMs += latencyMs;
                if (latencyMs < stats.minLatencyMs) stats.minLatencyMs = latencyMs;
                if (latencyMs > stats.maxLatencyMs) stats.maxLatencyMs = latencyMs;
            }

            if (stats.lastFrameMs != 0) {
                uint32_t frameDt = now - stats.lastFrameMs;
                stats.frameCount++;
                stats.sumFrameMs += frameDt;
                if (frameDt < stats.minFrameMs) stats.minFrameMs = frameDt;
                if (frameDt > stats.maxFrameMs) stats.maxFrameMs = frameDt;
            }
            stats.lastFrameMs = now;

            stats.lastPcmSamples = static_cast<uint32_t>(frame.pcm.size());
            stats.lastSampleRate = getAudioSampleRate();

            if ((now - stats.windowStartMs) >= 2000) {
                const uint32_t windowMs = now - stats.windowStartMs;
                const int32_t avgLatencyMs =
                    (stats.sampleCount > 0)
                        ? static_cast<int32_t>(stats.sumLatencyMs / stats.sampleCount)
                        : 0;
                const uint32_t avgFrameMs =
                    (stats.frameCount > 0)
                        ? static_cast<uint32_t>(stats.sumFrameMs / stats.frameCount)
                        : 0;
                const float fps =
                    (windowMs > 0)
                        ? (stats.frameCount * 1000.0f / static_cast<float>(windowMs))
                        : 0.0f;
                const uint32_t pcmMs =
                    (stats.lastSampleRate > 0)
                        ? static_cast<uint32_t>((stats.lastPcmSamples * 1000ULL) / stats.lastSampleRate)
                        : 0;

                FASTLED_DBG("Audio latency ms avg " << avgLatencyMs
                               << " min " << stats.minLatencyMs
                               << " max " << stats.maxLatencyMs
                               << " | frame ms avg " << avgFrameMs
                               << " max " << stats.maxFrameMs
                               << " | fps " << fps
                               << " | pcm " << stats.lastPcmSamples
                               << " (" << pcmMs << " ms) sr " << stats.lastSampleRate
                               << " | gate " << (noiseGateOpen ? 1 : 0)
                               << " | invalid " << stats.invalidCount);

                stats = LatencyStats();
                stats.windowStartMs = now;
            }
        }

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
        uint8_t limit = maxBins ? bin32.NUM_FFT_BINS : bin16.NUM_FFT_BINS;
        FASTLED_DBG("rmsRaw " << (f.rms_raw / 32768.0f)
                               << " rmsSm " << (f.rms / 32768.0f)
                               << " gate " << (noiseGateOpen ? 1 : 0));
        FASTLED_DBG("blockRms(raw) " << (int)lastBlockRms
                               << " openAt " << (int)cNoiseGateOpen
                               << " closeAt " << (int)cNoiseGateClose
                               << " valid " << lastValidSamples << "/" << lastClampedSamples);
        FASTLED_DBG("pcmMin " << lastPcmMin << " pcmMax " << lastPcmMax);
        FASTLED_DBG("autoGain " << (autoGain ? 1 : 0)
                               << " agCeil " << lastAutoGainCeil
                               << " agDesired " << lastAutoGainDesired
                               << " agVal " << autoGainValue
                               << " cAudioGain " << cAudioGain
                               << " gainLevel " << vizConfig.gainLevel);
        FASTLED_DBG("agCeilx1000 " << (lastAutoGainCeil * 1000.0f));
        FASTLED_DBG("rmsNorm " << f.rms_norm);
        FASTLED_DBG("busA._norm " << f.busA._norm);
        FASTLED_DBG("busB._norm " << f.busB._norm);
        FASTLED_DBG("busC._norm " << f.busC._norm);
        FASTLED_DBG("---------- ");

    }

} // namespace myAudio
