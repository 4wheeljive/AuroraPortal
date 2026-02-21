#pragma once

namespace myAudio {

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
        float autoGainTarget = 0.5f;   // P90 ceiling → rms_norm target for loud signals
        bool autoFloor = true;
        float autoFloorAlpha = 0.01f;
        float autoFloorMin = 0.0f;
        float autoFloorMax = 0.5f;
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
    }

    void updateAutoGain(float level) {
        if (!vizConfig.autoGain) {
            autoGainValue = 1.0f;
            return;
        }

        // Percentile-ceiling approach (preserves dynamic range) ---
        // Estimate the 90th percentile of recent input levels using
        // Robbins-Monro stochastic quantile estimation. Gain is set so
        // the ceiling maps to autoGainTarget in the final rms_norm output,
        // while quieter signals remain proportionally lower.
        static float ceilingEstimate = 0.02f;  // initial guess for P90 of [rmsNormRaw]
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

} // namespace myAudio
