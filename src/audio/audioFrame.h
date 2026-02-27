#pragma once

// Note: do NOT #include "audioBus.h" here.
// audioBus.h includes this file (after defining Bus) to break the mutual dependency.
// Bus is complete by the time this file is processed.

namespace myAudio {

    //=====================================================================
    // AUDIO FRAME - Per-frame snapshot helper
    // Ensures one audio sample -> one set of derived values per frame.
    //=====================================================================

    struct AudioFrame {
        bool valid = false;
        uint32_t timestamp = 0;
        float rms_raw = 0.0f;
        float rms = 0.0f;
        float rms_norm = 0.0f;
        float rms_factor = 0.0f;
        float rms_fast_norm = 0.0f;
        float energy = 0.0f;
        float peak = 0.0f;
        float peak_norm = 0.0f;
        float voxConf = 0.0f;
        float voxConfEMA = 0.0f;
        float smoothedVoxConf = 0.0f;
        float scaledVoxConf = 0.0f;
        float voxApprox = 0.0f;

        const fl::FFTBins* fft = nullptr;
        bool fft_norm_valid = false;
        float fft_pre[MAX_FFT_BINS] = {0};
        float fft_norm[MAX_FFT_BINS] = {0};
        fl::Slice<const int16_t> pcm;
        Bus busA;
        Bus busB;
        Bus busC;
    };

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
        
        // *** STAGE: Get frame RMS and calculate _norm and _factor values (audioAnalysis.h)
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
            }
        } else { // if frame not valid
            frame.fft = nullptr;
            frame.fft_norm_valid = false;
            frame.rms_norm = 0.0f;
            frame.peak_norm = 0.0f;
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
        finalizeBus(frame, busA, rmsPostFloor, gainAppliedLevel);
        finalizeBus(frame, busB, rmsPostFloor, gainAppliedLevel);
        finalizeBus(frame, busC, rmsPostFloor, gainAppliedLevel);
    
        frame.busA = busA;
        frame.busB = busB;
        frame.busC = busC;
       
        // *** STAGE: Return current AudioFrame frame to calling functions 
        // e.g., getAudio() --> updateAudioFrame() --> captureAudioFrame()  
        return frame;

    } // captureAudioFrame()

}