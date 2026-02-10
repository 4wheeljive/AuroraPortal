#pragma once

#include <cstdint>
#include <cmath>
#include "fl/math_macros.h"
#include "fl/fft.h"

namespace myAudio {

/**
 * Lightweight streaming beat tracker.
 *
 * Pipeline:
 *  1) Multi-band spectral flux (ODF) from FFT magnitudes
 *  2) ODF smoothing + DC removal
 *  3) Adaptive peak picking
 *  4) Tempo estimate from ODF autocorrelation (periodicity)
 *  5) Phase-locked beat acceptance + median BPM
 */
class BeatTracker {
public:
    //=========================================================================
    // Configuration
    //=========================================================================
    static constexpr uint8_t MAX_BINS = 32;
    static constexpr uint16_t ODF_HISTORY_SIZE = 256;  // ~4s at 60fps
    static constexpr uint8_t ODF_THRESH_WINDOW = 48;   // ~0.8s at 60fps
    static constexpr uint8_t BEAT_HISTORY_SIZE = 16;

    static constexpr float MIN_BPM = 50.0f;
    static constexpr float MAX_BPM = 200.0f;
    static constexpr uint32_t MIN_BEAT_INTERVAL_MS =
        static_cast<uint32_t>(60000.0f / MAX_BPM);

    static constexpr float BASS_WEIGHT = 1.0f;
    static constexpr float MID_WEIGHT = 0.60f;
    static constexpr float TREBLE_WEIGHT = 0.35f;

    static constexpr float TIME_ONSET_WEIGHT = 0.60f;
    static constexpr float TIME_ONSET_GAIN = 2.5f;

    static constexpr float ODF_SMOOTH_ALPHA = 0.20f;   // onset smoothing
    static constexpr float ODF_MEAN_ALPHA = 0.02f;     // DC removal time constant
    static constexpr float THRESH_STD_MULT = 1.6f;     // adaptive threshold
    static constexpr float MIN_ODF_THRESHOLD = 0.01f;

    static constexpr uint8_t TEMPO_UPDATE_INTERVAL = 8; // frames
    static constexpr float TEMPO_SMOOTH_ALPHA = 0.20f;

    //=========================================================================
    // Public interface
    //=========================================================================
    void update(const fl::FFTBins* fft,
                uint8_t numBins,
                uint8_t firstMidBin,
                uint8_t firstTrebleBin,
                uint32_t timestampMs,
                float floorDbNorm,
                bool gateOpen,
                float timeEnergy) {
        mBeatDetected = false;

        updateFrameDt(timestampMs);

        float flux = 0.0f;
        if (fft && numBins > 0 && fft->bins_db.size() > 0) {
            flux = computeSpectralFlux(*fft, numBins, firstMidBin, firstTrebleBin, floorDbNorm);
        }

        float timeOnset = 0.0f;
        if (timeEnergy > 0.0f) {
            float delta = timeEnergy - mPrevTimeEnergy;
            if (delta > 0.0f) {
                timeOnset = delta * TIME_ONSET_GAIN;
                timeOnset = clampf(timeOnset, 0.0f, 1.0f);
            }
        }
        mPrevTimeEnergy = timeEnergy;

        flux += timeOnset * TIME_ONSET_WEIGHT;

        // Smooth and remove DC from ODF
        mFluxSmooth += ODF_SMOOTH_ALPHA * (flux - mFluxSmooth);
        mFluxMean += ODF_MEAN_ALPHA * (mFluxSmooth - mFluxMean);

        float odf = mFluxSmooth - mFluxMean;
        if (odf < 0.0f) odf = 0.0f;
        if (!gateOpen) {
            odf = 0.0f;
        }

        pushOdf(odf);

        // Adaptive threshold from recent ODF history
        float threshold = computeThreshold();

        // Periodicity-based tempo estimate
        if (mOdfCount >= 32) {
            mTempoUpdateCounter++;
            if (mTempoUpdateCounter >= TEMPO_UPDATE_INTERVAL) {
                updateTempoEstimate();
                mTempoUpdateCounter = 0;
            }
        }

        // Peak picking on ODF (local maxima)
        bool isPeak = (mOdfPrev1 > mOdfPrev2) && (mOdfPrev1 > odf) && (mOdfPrev1 > threshold);
        if (isPeak && gateOpen) {
            uint32_t elapsed = (mLastBeatTime > 0) ? (timestampMs - mLastBeatTime) : 999999;
            if (elapsed >= MIN_BEAT_INTERVAL_MS) {
                bool accept = true;
                if (mTempoMs > 0.0f && mTempoConfidence > 0.25f) {
                    float tol = clampf(mTempoMs * 0.20f, 40.0f, 120.0f);
                    float diff = std::fabs(static_cast<float>(elapsed) - mTempoMs);
                    if (diff > tol && elapsed < mTempoMs * 1.5f) {
                        accept = false;
                    }
                }
                if (accept) {
                    recordBeat(timestampMs);
                }
            }
        }

        mOdfPrev2 = mOdfPrev1;
        mOdfPrev1 = odf;

        // Debug values
        mLastThreshold = threshold;
        mLastOdf = odf;
        mLastFlux = flux;
    }

    bool isBeat() const { return mBeatDetected; }
    float getBPM() const { return mSmoothedBPM; }
    float getRawBPM() const { return mCurrentBPM; }
    float getConfidence() const { return mConfidence; }
    float getTempoConfidence() const { return mTempoConfidence; }
    uint32_t getLastBeatTime() const { return mLastBeatTime; }
    uint32_t getBeatCount() const { return mBeatNumber; }

    // Debug accessors
    float getLastThreshold() const { return mLastThreshold; }
    float getLastOdf() const { return mLastOdf; }
    float getLastFlux() const { return mLastFlux; }
    float getTempoMs() const { return mTempoMs; }

    void reset() {
        for (uint8_t i = 0; i < MAX_BINS; i++) {
            mPrevMag[i] = 0.0f;
        }
        for (uint16_t i = 0; i < ODF_HISTORY_SIZE; i++) {
            mOdfHistory[i] = 0.0f;
        }
        for (uint8_t i = 0; i < BEAT_HISTORY_SIZE; i++) {
            mBeatIntervals[i] = 0;
        }

        mOdfIndex = 0;
        mOdfCount = 0;
        mBeatIndex = 0;
        mBeatCount = 0;
        mBeatNumber = 0;
        mLastBeatTime = 0;
        mBeatDetected = false;

        mFluxSmooth = 0.0f;
        mFluxMean = 0.0f;
        mOdfPrev1 = 0.0f;
        mOdfPrev2 = 0.0f;
        mPrevTimeEnergy = 0.0f;

        mLastFrameTime = 0;
        mFrameDtMs = 0.0f;

        mTempoMs = 0.0f;
        mTempoConfidence = 0.0f;
        mTempoUpdateCounter = 0;

        mCurrentBPM = 0.0f;
        mSmoothedBPM = 0.0f;
        mConfidence = 0.0f;

        mLastThreshold = 0.0f;
        mLastOdf = 0.0f;
        mLastFlux = 0.0f;
    }

private:
    //=========================================================================
    // Helpers
    //=========================================================================
    static inline float clampf(float v, float lo, float hi) {
        return (v < lo) ? lo : (v > hi) ? hi : v;
    }

    void updateFrameDt(uint32_t timestampMs) {
        if (mLastFrameTime > 0 && timestampMs > mLastFrameTime) {
            uint32_t dt = timestampMs - mLastFrameTime;
            if (mFrameDtMs <= 0.0f) {
                mFrameDtMs = static_cast<float>(dt);
            } else {
                mFrameDtMs = mFrameDtMs * 0.90f + static_cast<float>(dt) * 0.10f;
            }
        }
        mLastFrameTime = timestampMs;
    }

    float computeSpectralFlux(const fl::FFTBins& fft,
                              uint8_t numBins,
                              uint8_t firstMidBin,
                              uint8_t firstTrebleBin,
                              float floorDbNorm) {
        uint8_t bins = numBins;
        if (bins > MAX_BINS) bins = MAX_BINS;

        uint8_t available = static_cast<uint8_t>(fft.bins_db.size());
        if (available < bins) bins = available;
        if (bins == 0) return 0.0f;

        float flux = 0.0f;
        float totalWeight = 0.0f;

        for (uint8_t i = 0; i < bins; i++) {
            float mag = fft.bins_db[i] / 100.0f;
            mag = FL_MAX(0.0f, mag - floorDbNorm);
            mag = clampf(mag, 0.0f, 1.0f);

            float delta = mag - mPrevMag[i];
            float weight = (i < firstMidBin) ? BASS_WEIGHT
                           : (i < firstTrebleBin) ? MID_WEIGHT
                           : TREBLE_WEIGHT;

            if (delta > 0.0f) {
                flux += delta * weight;
            }
            mPrevMag[i] = mag;
            totalWeight += weight;
        }

        if (totalWeight > 0.0f) {
            flux /= totalWeight;
        }
        return flux;
    }

    void pushOdf(float odf) {
        mOdfHistory[mOdfIndex] = odf;
        mOdfIndex = (mOdfIndex + 1) % ODF_HISTORY_SIZE;
        if (mOdfCount < ODF_HISTORY_SIZE) {
            mOdfCount++;
        }
    }

    float computeThreshold() const {
        uint16_t window = (mOdfCount < ODF_THRESH_WINDOW) ? mOdfCount : ODF_THRESH_WINDOW;
        if (window == 0) return MIN_ODF_THRESHOLD;

        uint16_t start = (mOdfIndex + ODF_HISTORY_SIZE - window) % ODF_HISTORY_SIZE;
        float mean = 0.0f;
        for (uint16_t i = 0; i < window; i++) {
            mean += mOdfHistory[(start + i) % ODF_HISTORY_SIZE];
        }
        mean /= static_cast<float>(window);

        float var = 0.0f;
        for (uint16_t i = 0; i < window; i++) {
            float v = mOdfHistory[(start + i) % ODF_HISTORY_SIZE] - mean;
            var += v * v;
        }
        var /= static_cast<float>(window);
        float stddev = std::sqrt(var);

        float threshold = mean + THRESH_STD_MULT * stddev;
        return (threshold > MIN_ODF_THRESHOLD) ? threshold : MIN_ODF_THRESHOLD;
    }

    void updateTempoEstimate() {
        if (mOdfCount < 32 || mFrameDtMs <= 0.0f) {
            return;
        }

        float minPeriodMs = 60000.0f / MAX_BPM;
        float maxPeriodMs = 60000.0f / MIN_BPM;

        uint16_t minLag = static_cast<uint16_t>(clampf(minPeriodMs / mFrameDtMs + 0.5f, 1.0f, static_cast<float>(ODF_HISTORY_SIZE - 1)));
        uint16_t maxLag = static_cast<uint16_t>(clampf(maxPeriodMs / mFrameDtMs + 0.5f, 1.0f, static_cast<float>(ODF_HISTORY_SIZE - 1)));

        if (maxLag <= minLag + 2) {
            return;
        }
        if (maxLag >= mOdfCount) {
            maxLag = mOdfCount - 1;
        }

        uint16_t start = (mOdfIndex + ODF_HISTORY_SIZE - mOdfCount) % ODF_HISTORY_SIZE;
        auto odfAt = [&](uint16_t i) -> float {
            return mOdfHistory[(start + i) % ODF_HISTORY_SIZE];
        };

        float bestCorr = -1.0f;
        uint16_t bestLag = 0;
        float sumCorr = 0.0f;
        uint16_t lagCount = 0;

        for (uint16_t lag = minLag; lag <= maxLag; lag++) {
            float sum = 0.0f;
            uint16_t n = mOdfCount - lag;
            for (uint16_t i = lag; i < mOdfCount; i++) {
                sum += odfAt(i) * odfAt(i - lag);
            }
            float corr = (n > 0) ? (sum / static_cast<float>(n)) : 0.0f;
            sumCorr += corr;
            lagCount++;
            if (corr > bestCorr) {
                bestCorr = corr;
                bestLag = lag;
            }
        }

        if (bestLag > 0 && lagCount > 0 && bestCorr > 0.0f) {
            float avgCorr = sumCorr / static_cast<float>(lagCount);
            float confidence = clampf((bestCorr - avgCorr) / bestCorr, 0.0f, 1.0f);
            float tempoMs = static_cast<float>(bestLag) * mFrameDtMs;

            if (mTempoMs <= 0.0f) {
                mTempoMs = tempoMs;
            } else {
                mTempoMs = mTempoMs * (1.0f - TEMPO_SMOOTH_ALPHA) + tempoMs * TEMPO_SMOOTH_ALPHA;
            }
            mTempoConfidence = mTempoConfidence * 0.80f + confidence * 0.20f;
        }
    }

    void recordBeat(uint32_t timestampMs) {
        uint32_t prevBeatTime = mLastBeatTime;
        mLastBeatTime = timestampMs;
        mBeatNumber++;
        mBeatDetected = true;

        if (prevBeatTime > 0) {
            uint32_t interval = timestampMs - prevBeatTime;
            if (interval >= MIN_BEAT_INTERVAL_MS && interval <= static_cast<uint32_t>(60000.0f / MIN_BPM)) {
                mBeatIntervals[mBeatIndex] = interval;
                mBeatIndex = (mBeatIndex + 1) % BEAT_HISTORY_SIZE;
                if (mBeatCount < BEAT_HISTORY_SIZE) {
                    mBeatCount++;
                }
                updateBPM();
            }
        }
    }

    void updateBPM() {
        if (mBeatCount < 4) {
            mCurrentBPM = 0.0f;
            return;
        }

        uint32_t sorted[BEAT_HISTORY_SIZE];
        for (uint8_t i = 0; i < mBeatCount; i++) {
            sorted[i] = mBeatIntervals[i];
        }

        for (uint8_t i = 1; i < mBeatCount; i++) {
            uint32_t key = sorted[i];
            int8_t j = i - 1;
            while (j >= 0 && sorted[j] > key) {
                sorted[j + 1] = sorted[j];
                j--;
            }
            sorted[j + 1] = key;
        }

        uint32_t medianInterval;
        if (mBeatCount % 2 == 0) {
            medianInterval = (sorted[mBeatCount / 2 - 1] + sorted[mBeatCount / 2]) / 2;
        } else {
            medianInterval = sorted[mBeatCount / 2];
        }

        if (medianInterval > 0) {
            mCurrentBPM = 60000.0f / static_cast<float>(medianInterval);
        }

        uint8_t q1Idx = mBeatCount / 4;
        uint8_t q3Idx = (mBeatCount * 3) / 4;
        float q1 = static_cast<float>(sorted[q1Idx]);
        float q3 = static_cast<float>(sorted[q3Idx]);
        float iqr = q3 - q1;
        float median = static_cast<float>(medianInterval);
        float niqr = (median > 0.0f) ? iqr / median : 1.0f;
        mConfidence = (niqr < 0.5f) ? clampf(1.0f - (niqr * 2.0f), 0.0f, 1.0f) : 0.0f;

        if (mSmoothedBPM > 0.0f) {
            float ratio = mCurrentBPM / mSmoothedBPM;
            float alpha = (ratio > 0.7f && ratio < 1.4f) ? 0.10f : 0.35f;
            mSmoothedBPM += alpha * (mCurrentBPM - mSmoothedBPM);
        } else {
            mSmoothedBPM = mCurrentBPM;
        }
    }

    //=========================================================================
    // State
    //=========================================================================
    float mPrevMag[MAX_BINS] = {0.0f};

    float mOdfHistory[ODF_HISTORY_SIZE] = {0.0f};
    uint16_t mOdfIndex = 0;
    uint16_t mOdfCount = 0;

    float mFluxSmooth = 0.0f;
    float mFluxMean = 0.0f;

    float mOdfPrev2 = 0.0f;
    float mOdfPrev1 = 0.0f;
    float mPrevTimeEnergy = 0.0f;

    uint32_t mLastFrameTime = 0;
    float mFrameDtMs = 0.0f;

    float mTempoMs = 0.0f;
    float mTempoConfidence = 0.0f;
    uint8_t mTempoUpdateCounter = 0;

    uint32_t mBeatIntervals[BEAT_HISTORY_SIZE] = {0};
    uint8_t mBeatIndex = 0;
    uint8_t mBeatCount = 0;

    uint32_t mLastBeatTime = 0;
    uint32_t mBeatNumber = 0;
    bool mBeatDetected = false;

    float mCurrentBPM = 0.0f;
    float mSmoothedBPM = 0.0f;
    float mConfidence = 0.0f;

    float mLastThreshold = 0.0f;
    float mLastOdf = 0.0f;
    float mLastFlux = 0.0f;
};

} // namespace myAudio
