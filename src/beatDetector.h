#pragma once

#include <cstdint>
#include "fl/math_macros.h"

namespace myAudio {

/**
 * Bass-focused beat detector with stable BPM estimation.
 *
 * Design principles:
 * - Uses spectral flux (change in bass energy) to detect onsets
 * - Adaptive threshold based on recent flux history
 * - Calculates BPM from median inter-beat intervals
 * - Smooths BPM output to resist noise once locked on
 *
 * Usage:
 *   beatDetector.update(bassEnergy);  // Call every frame with bass energy (0-1)
 *   if (beatDetector.isBeat()) { ... }
 *   float bpm = beatDetector.getBPM();
 */
class BeatDetector {
public:
    //=========================================================================
    // Configuration
    //=========================================================================

    static constexpr uint8_t FLUX_HISTORY_SIZE = 32;     // For adaptive threshold (~0.5s at 60fps)
    static constexpr uint8_t BEAT_HISTORY_SIZE = 16;     // Track last 16 beats for BPM
    static constexpr uint32_t MIN_BEAT_INTERVAL_MS = 350; // Max 171 BPM (prevents double-triggers)
    static constexpr uint32_t MAX_BEAT_INTERVAL_MS = 1500; // Min 40 BPM
    static constexpr float THRESHOLD_MULTIPLIER = 2.5f;   // How much above average flux = beat
    static constexpr float MIN_FLUX_THRESHOLD = 0.02f;    // Minimum flux to trigger beat
    static constexpr float BPM_SMOOTHING = 0.90f;         // How much to weight previous BPM (0-1)

    //=========================================================================
    // Public interface
    //=========================================================================

    /**
     * Update detector with current bass energy.
     * @param bassEnergy Normalized bass energy (0.0 - 1.0), typically from FFT bins 0-3
     * @param timestampMs Current time in milliseconds
     */
    void update(float bassEnergy, uint32_t timestampMs) {
        // Calculate spectral flux (positive change in energy = onset)
        float flux = bassEnergy - mPreviousEnergy;
        if (flux < 0.0f) flux = 0.0f;  // Only care about increases
        mPreviousEnergy = bassEnergy;

        // Store flux in history for adaptive threshold
        mFluxHistory[mFluxIndex] = flux;
        mFluxIndex = (mFluxIndex + 1) % FLUX_HISTORY_SIZE;
        if (mFluxCount < FLUX_HISTORY_SIZE) {
            mFluxCount++;
        }

        // Calculate adaptive threshold from mean flux
        float sumFlux = 0.0f;
        for (uint8_t i = 0; i < mFluxCount; i++) {
            sumFlux += mFluxHistory[i];
        }
        float meanFlux = sumFlux / mFluxCount;
        float threshold = fl::fl_max(meanFlux * THRESHOLD_MULTIPLIER, MIN_FLUX_THRESHOLD);

        // Detect onset: flux exceeds threshold AND we're not in refractory period
        uint32_t timeSinceLastBeat = timestampMs - mLastBeatTime;
        bool aboveThreshold = flux > threshold;
        bool canTrigger = timeSinceLastBeat >= MIN_BEAT_INTERVAL_MS;

        // Beat detection with rising edge
        mBeatDetected = false;
        if (aboveThreshold && canTrigger && !mWasAboveThreshold) {
            mBeatDetected = true;

            // Record inter-beat interval for BPM calculation
            if (mLastBeatTime > 0 && timeSinceLastBeat <= MAX_BEAT_INTERVAL_MS) {
                mBeatIntervals[mBeatIndex] = timeSinceLastBeat;
                mBeatIndex = (mBeatIndex + 1) % BEAT_HISTORY_SIZE;
                if (mBeatCount < BEAT_HISTORY_SIZE) {
                    mBeatCount++;
                }
                updateBPM();
            }

            mLastBeatTime = timestampMs;
            mBeatNumber++;
        }

        mWasAboveThreshold = aboveThreshold;

        // Store debug values
        mLastThreshold = threshold;
        mLastFlux = flux;
        mLastEnergy = bassEnergy;
    }

    /** Returns true if a beat was detected this frame */
    bool isBeat() const { return mBeatDetected; }

    /** Returns current BPM estimate (0 if not enough data) */
    float getBPM() const { return mSmoothedBPM; }

    /** Returns raw (unsmoothed) BPM for debugging */
    float getRawBPM() const { return mCurrentBPM; }

    /** Returns confidence in BPM estimate (0-1) */
    float getConfidence() const { return mConfidence; }

    /** Total beats detected since start */
    uint32_t getBeatCount() const { return mBeatNumber; }

    /** Time of last beat in ms */
    uint32_t getLastBeatTime() const { return mLastBeatTime; }

    // Debug accessors
    float getLastThreshold() const { return mLastThreshold; }
    float getLastFlux() const { return mLastFlux; }
    float getLastEnergy() const { return mLastEnergy; }

    /** Reset all state */
    void reset() {
        mFluxIndex = 0;
        mFluxCount = 0;
        mBeatIndex = 0;
        mBeatCount = 0;
        mBeatNumber = 0;
        mLastBeatTime = 0;
        mBeatDetected = false;
        mWasAboveThreshold = false;
        mPreviousEnergy = 0.0f;
        mCurrentBPM = 0.0f;
        mSmoothedBPM = 0.0f;
        mConfidence = 0.0f;
        for (uint8_t i = 0; i < FLUX_HISTORY_SIZE; i++) {
            mFluxHistory[i] = 0.0f;
        }
        for (uint8_t i = 0; i < BEAT_HISTORY_SIZE; i++) {
            mBeatIntervals[i] = 0;
        }
    }

private:
    //=========================================================================
    // BPM calculation from inter-beat intervals
    //=========================================================================

    void updateBPM() {
        if (mBeatCount < 4) {
            // Need at least 4 intervals for meaningful BPM
            mCurrentBPM = 0.0f;
            return;
        }

        // Copy intervals for sorting (to find median)
        uint32_t sorted[BEAT_HISTORY_SIZE];
        for (uint8_t i = 0; i < mBeatCount; i++) {
            sorted[i] = mBeatIntervals[i];
        }

        // Simple insertion sort (small array)
        for (uint8_t i = 1; i < mBeatCount; i++) {
            uint32_t key = sorted[i];
            int8_t j = i - 1;
            while (j >= 0 && sorted[j] > key) {
                sorted[j + 1] = sorted[j];
                j--;
            }
            sorted[j + 1] = key;
        }

        // Use median interval (robust to outliers)
        uint32_t medianInterval;
        if (mBeatCount % 2 == 0) {
            medianInterval = (sorted[mBeatCount/2 - 1] + sorted[mBeatCount/2]) / 2;
        } else {
            medianInterval = sorted[mBeatCount/2];
        }

        // Convert interval to BPM: BPM = 60000 / interval_ms
        if (medianInterval > 0) {
            mCurrentBPM = 60000.0f / static_cast<float>(medianInterval);
        }

        // Calculate confidence based on interval consistency (IQR method)
        // Lower quartile (Q1) and upper quartile (Q3)
        uint8_t q1Idx = mBeatCount / 4;
        uint8_t q3Idx = (mBeatCount * 3) / 4;
        float q1 = static_cast<float>(sorted[q1Idx]);
        float q3 = static_cast<float>(sorted[q3Idx]);
        float iqr = q3 - q1;
        float median = static_cast<float>(medianInterval);

        // Normalized IQR (relative to median)
        float niqr = (median > 0) ? iqr / median : 1.0f;

        // Convert to confidence: niqr=0 -> conf=1, niqr=0.3 -> conf=0.5
        mConfidence = fl::fl_max(0.0f, fl::fl_min(1.0f, 1.0f - (niqr * 2.0f)));

        // Apply smoothing to BPM output
        // Only smooth if we have a previous value and new value is reasonably close
        if (mSmoothedBPM > 0.0f) {
            float ratio = mCurrentBPM / mSmoothedBPM;
            if (ratio > 0.7f && ratio < 1.4f) {
                // Within 30% - smooth it
                mSmoothedBPM = mSmoothedBPM * BPM_SMOOTHING + mCurrentBPM * (1.0f - BPM_SMOOTHING);
            } else {
                // Large change - might be tempo change, use less smoothing
                mSmoothedBPM = mSmoothedBPM * 0.5f + mCurrentBPM * 0.5f;
            }
        } else {
            mSmoothedBPM = mCurrentBPM;
        }
    }

    //=========================================================================
    // State
    //=========================================================================

    // Flux history for adaptive threshold
    float mFluxHistory[FLUX_HISTORY_SIZE] = {0};
    uint8_t mFluxIndex = 0;
    uint8_t mFluxCount = 0;

    // Beat interval history for BPM calculation
    uint32_t mBeatIntervals[BEAT_HISTORY_SIZE] = {0};
    uint8_t mBeatIndex = 0;
    uint8_t mBeatCount = 0;

    // Beat detection state
    float mPreviousEnergy = 0.0f;
    uint32_t mLastBeatTime = 0;
    uint32_t mBeatNumber = 0;
    bool mBeatDetected = false;
    bool mWasAboveThreshold = false;

    // Output values
    float mCurrentBPM = 0.0f;
    float mSmoothedBPM = 0.0f;
    float mConfidence = 0.0f;

    // Debug values
    float mLastThreshold = 0.0f;
    float mLastFlux = 0.0f;
    float mLastEnergy = 0.0f;
};

} // namespace myAudio
