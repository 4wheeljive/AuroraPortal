#pragma once

// =====================================================
// THIS IS EARLY-DEVELOPMENT EXPERIMENTAL STUFF
// USE AT YOUR OWN RISK! ;-)
// =====================================================

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
    static constexpr float DEFAULT_MIN_BPM = 50.0f;
    static constexpr float DEFAULT_MAX_BPM = 200.0f;

    static constexpr float DEFAULT_BASS_WEIGHT = 1.0f;
    static constexpr float DEFAULT_MID_WEIGHT = 0.60f;
    static constexpr float DEFAULT_TREBLE_WEIGHT = 0.35f;

    static constexpr float TIME_ONSET_WEIGHT = 0.30f;
    static constexpr float TIME_ONSET_GAIN = 2.5f;
    static constexpr float TEMPO_TIME_ONSET_WEIGHT = 0.0f;

    static constexpr float TEMPO_BASS_WEIGHT_SCALE = 1.0f;
    static constexpr float TEMPO_MID_WEIGHT_SCALE = 0.35f;
    static constexpr float TEMPO_TREBLE_WEIGHT_SCALE = 0.10f;
    static constexpr float TEMPO_ODF_SMOOTH_SCALE = 0.6f;
    static constexpr float TEMPO_ODF_MEAN_SCALE = 0.6f;

    static constexpr float DEFAULT_ODF_SMOOTH_ALPHA = 0.20f;   // onset smoothing
    static constexpr float DEFAULT_ODF_MEAN_ALPHA = 0.02f;     // DC removal time constant
    static constexpr float DEFAULT_THRESH_STD_MULT = 1.6f;     // adaptive threshold
    static constexpr float DEFAULT_MIN_ODF_THRESHOLD = 0.01f;

    static constexpr uint8_t DEFAULT_TEMPO_UPDATE_INTERVAL = 8; // frames
    static constexpr float DEFAULT_TEMPO_SMOOTH_ALPHA = 0.20f;
    static constexpr float TEMPO_LOCK_CONFIDENCE = 0.35f;
    static constexpr float TEMPO_LOCK_TOL_FRAC = 0.18f;
    static constexpr float TEMPO_MIN_VAR_SCALE = 0.5f;
    static constexpr float TEMPO_PLL_ALPHA = 0.12f;
    static constexpr float BPM_SMOOTH_ALPHA = 0.12f;
    static constexpr float TEMPO_LAG_PEAK_FRACTION = 0.88f;
    static constexpr uint8_t INTERVAL_HISTORY_SIZE = 8;

    //=========================================================================
    // Public interface
    //=========================================================================
    void update(const fl::FFTBins* fft,
                const float* preBins,
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
        float tempoFlux = 0.0f;
        if (preBins && numBins > 0) {
            computeFluxesPreBins(preBins, numBins, firstMidBin, firstTrebleBin, flux, tempoFlux);
        } else if (fft && numBins > 0 && fft->bins_db.size() > 0) {
            computeFluxes(*fft, numBins, firstMidBin, firstTrebleBin, floorDbNorm, flux, tempoFlux);
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
        tempoFlux += timeOnset * TEMPO_TIME_ONSET_WEIGHT;

        // Smooth and remove DC from ODF
        mFluxSmooth += mOdfSmoothAlpha * (flux - mFluxSmooth);
        mFluxMean += mOdfMeanAlpha * (mFluxSmooth - mFluxMean);

        float odf = mFluxSmooth - mFluxMean;
        if (odf < 0.0f) odf = 0.0f;
        if (!gateOpen) {
            odf = 0.0f;
        }

        pushOdf(odf);

        // Separate, bass-biased ODF for tempo estimation
        float tempoSmoothAlpha = mOdfSmoothAlpha * TEMPO_ODF_SMOOTH_SCALE;
        float tempoMeanAlpha = mOdfMeanAlpha * TEMPO_ODF_MEAN_SCALE;
        tempoSmoothAlpha = clampf(tempoSmoothAlpha, 0.005f, 0.5f);
        tempoMeanAlpha = clampf(tempoMeanAlpha, 0.001f, 0.2f);
        mTempoFluxSmooth += tempoSmoothAlpha * (tempoFlux - mTempoFluxSmooth);
        mTempoFluxMean += tempoMeanAlpha * (mTempoFluxSmooth - mTempoFluxMean);

        float tempoOdf = mTempoFluxSmooth - mTempoFluxMean;
        if (tempoOdf < 0.0f) tempoOdf = 0.0f;
        if (!gateOpen) {
            tempoOdf = 0.0f;
        }
        pushTempoOdf(tempoOdf);

        // Adaptive threshold from recent ODF history
        float threshold = computeThreshold();

        // Periodicity-based tempo estimate
        bool allowTempoEstimate = (mTempoMs <= 0.0f);
        if (mIntervalCount < 4) {
            allowTempoEstimate = true;
        }
        if (mTempoConfidence < 0.40f) {
            allowTempoEstimate = true;
        }
        if (mIntervalCount >= 6 && mTempoConfidence > 0.70f) {
            allowTempoEstimate = false;
        }
        if (allowTempoEstimate && mTempoOdfCount >= 32) {
            mTempoUpdateCounter++;
            if (mTempoUpdateCounter >= mTempoUpdateInterval) {
                updateTempoEstimate();
                mTempoUpdateCounter = 0;
            }
        }

        bool tempoLocked = (mTempoMs > 0.0f && mTempoConfidence > TEMPO_LOCK_CONFIDENCE);
        float tempoTol = 0.0f;
        if (tempoLocked) {
            tempoTol = clampf(mTempoMs * TEMPO_LOCK_TOL_FRAC, 35.0f, 120.0f);
            if (mNextBeatTime <= 0.0f) {
                mNextBeatTime = static_cast<float>(timestampMs) + mTempoMs;
            } else if (static_cast<float>(timestampMs) > mNextBeatTime + tempoTol) {
                float skips = std::floor((static_cast<float>(timestampMs) - mNextBeatTime) / mTempoMs) + 1.0f;
                mNextBeatTime += skips * mTempoMs;
            }
        }

        // Peak picking on ODF (local maxima)
        bool isPeak = (mOdfPrev1 > mOdfPrev2) && (mOdfPrev1 > odf) && (mOdfPrev1 > threshold);
        if (isPeak && gateOpen) {
            uint32_t elapsed = (mLastBeatTime > 0) ? (timestampMs - mLastBeatTime) : 999999;
            uint32_t minInterval = mMinBeatIntervalMs;
            if (mTempoMs > 0.0f) {
                uint32_t tempoMin = static_cast<uint32_t>(mTempoMs * 0.55f);
                if (tempoMin > minInterval) {
                    minInterval = tempoMin;
                }
            }
            if (elapsed >= minInterval) {
                bool accept = true;
                bool strongPeak = (mOdfPrev1 > (threshold * 1.6f));
                if (tempoLocked) {
                    float phaseError = static_cast<float>(timestampMs) - mNextBeatTime;
                    if (std::fabs(phaseError) > tempoTol) {
                        // Allow strong peaks within half a period to re-capture
                        // when the phase/tempo has drifted.
                        if (!(strongPeak && std::fabs(phaseError) < (mTempoMs * 0.5f))) {
                            accept = false;
                        }
                    }
                } else if (mTempoMs > 0.0f && mTempoConfidence > 0.25f) {
                    float tol = clampf(mTempoMs * 0.20f, 40.0f, 120.0f);
                    float diff = std::fabs(static_cast<float>(elapsed) - mTempoMs);
                    if (diff > tol && elapsed < mTempoMs * 1.5f) {
                        accept = false;
                    }
                }
                if (accept) {
                    recordBeat(timestampMs);
                    if (tempoLocked) {
                        float phaseError = static_cast<float>(timestampMs) - mNextBeatTime;
                        float correction = clampf(phaseError, -tempoTol, tempoTol) * 0.2f;
                        mNextBeatTime = static_cast<float>(timestampMs) + mTempoMs + correction;
                    }
                }
            }
        }

        updateBpmFromTempo();

        if (mLastBeatTime > 0) {
            uint32_t silenceMs = timestampMs - mLastBeatTime;
            if (silenceMs > (mMaxBeatIntervalMs * 2U)) {
                mTempoMs = 0.0f;
                mTempoConfidence = 0.0f;
                mNextBeatTime = 0.0f;
                mCurrentBPM = 0.0f;
                mSmoothedBPM = 0.0f;
                mConfidence = 0.0f;
                mBeatIntervalCount = 0;
            }
        }

        mOdfPrev2 = mOdfPrev1;
        mOdfPrev1 = odf;

        // Debug values
        mLastThreshold = threshold;
        mLastOdf = odf;
        mLastFlux = flux;
        mLastTempoOdf = tempoOdf;
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
    float getLastTempoOdf() const { return mLastTempoOdf; }
    float getTempoMs() const { return mTempoMs; }

    void setConfig(float minBpm,
                   float maxBpm,
                   float bassWeight,
                   float midWeight,
                   float trebleWeight,
                   float odfSmoothAlpha,
                   float odfMeanAlpha,
                   float threshStdMult,
                   float minOdfThreshold,
                   uint8_t tempoUpdateInterval,
                   float tempoSmoothAlpha) {
        float newMinBpm = clampf(minBpm, 30.0f, 300.0f);
        float newMaxBpm = clampf(maxBpm, 30.0f, 300.0f);
        bool rangeChanged = (std::fabs(newMinBpm - mMinBpm) > 0.01f) ||
                            (std::fabs(newMaxBpm - mMaxBpm) > 0.01f);

        mMinBpm = newMinBpm;
        mMaxBpm = newMaxBpm;
        if (mMaxBpm < mMinBpm + 1.0f) {
            mMaxBpm = mMinBpm + 1.0f;
        }

        mBassWeight = clampf(bassWeight, 0.0f, 2.0f);
        mMidWeight = clampf(midWeight, 0.0f, 2.0f);
        mTrebleWeight = clampf(trebleWeight, 0.0f, 2.0f);

        mOdfSmoothAlpha = clampf(odfSmoothAlpha, 0.01f, 0.9f);
        mOdfMeanAlpha = clampf(odfMeanAlpha, 0.001f, 0.3f);

        mThreshStdMult = clampf(threshStdMult, 0.1f, 6.0f);
        mMinOdfThreshold = clampf(minOdfThreshold, 0.0f, 1.0f);

        mTempoUpdateInterval = (tempoUpdateInterval < 1) ? 1 : tempoUpdateInterval;
        mTempoSmoothAlpha = clampf(tempoSmoothAlpha, 0.01f, 0.9f);

        mMinBeatIntervalMs = static_cast<uint32_t>(60000.0f / mMaxBpm);
        mMaxBeatIntervalMs = static_cast<uint32_t>(60000.0f / mMinBpm);
        if (rangeChanged) {
            resetTempoState();
        }
    }

    void reset() {
        for (uint8_t i = 0; i < MAX_BINS; i++) {
            mPrevMag[i] = 0.0f;
        }
        for (uint16_t i = 0; i < ODF_HISTORY_SIZE; i++) {
            mOdfHistory[i] = 0.0f;
            mTempoOdfHistory[i] = 0.0f;
        }
        mOdfIndex = 0;
        mOdfCount = 0;
        mTempoOdfIndex = 0;
        mTempoOdfCount = 0;
        mBeatNumber = 0;
        mLastBeatTime = 0;
        mBeatDetected = false;
        mNextBeatTime = 0.0f;
        mBeatIntervalCount = 0;
        mIntervalIndex = 0;
        mIntervalCount = 0;
        for (uint8_t i = 0; i < INTERVAL_HISTORY_SIZE; i++) {
            mIntervalHistory[i] = 0.0f;
        }

        mFluxSmooth = 0.0f;
        mFluxMean = 0.0f;
        mTempoFluxSmooth = 0.0f;
        mTempoFluxMean = 0.0f;
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
        mLastTempoOdf = 0.0f;
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

    void computeFluxes(const fl::FFTBins& fft,
                       uint8_t numBins,
                       uint8_t firstMidBin,
                       uint8_t firstTrebleBin,
                       float floorDbNorm,
                       float& fullFlux,
                       float& tempoFlux) {
        (void)floorDbNorm;
        uint8_t bins = numBins;
        if (bins > MAX_BINS) bins = MAX_BINS;

        uint8_t available = static_cast<uint8_t>(fft.bins_raw.size());
        if (available < bins) bins = available;
        if (bins == 0) {
            fullFlux = 0.0f;
            tempoFlux = 0.0f;
            return;
        }

        float flux = 0.0f;
        float tFlux = 0.0f;
        float totalWeight = 0.0f;
        float totalTempoWeight = 0.0f;

        for (uint8_t i = 0; i < bins; i++) {
            float mag = fft.bins_raw[i];
            if (mag < 0.0f) mag = 0.0f;
            // Mild log compression to stabilize dynamic range.
            mag = std::log1pf(mag);

            float delta = mag - mPrevMag[i];
            float weight = (i < firstMidBin) ? mBassWeight
                           : (i < firstTrebleBin) ? mMidWeight
                           : mTrebleWeight;
            float tempoWeight = weight;
            if (i < firstMidBin) {
                tempoWeight *= TEMPO_BASS_WEIGHT_SCALE;
            } else if (i < firstTrebleBin) {
                tempoWeight *= TEMPO_MID_WEIGHT_SCALE;
            } else {
                tempoWeight *= TEMPO_TREBLE_WEIGHT_SCALE;
            }

            if (delta > 0.0f) {
                flux += delta * weight;
                tFlux += delta * tempoWeight;
            }
            mPrevMag[i] = mag;
            totalWeight += weight;
            totalTempoWeight += tempoWeight;
        }

        if (totalWeight > 0.0f) {
            flux /= totalWeight;
        }
        if (totalTempoWeight > 0.0f) {
            tFlux /= totalTempoWeight;
        }
        fullFlux = flux;
        tempoFlux = tFlux;
    }

    void computeFluxesPreBins(const float* bins,
                              uint8_t numBins,
                              uint8_t firstMidBin,
                              uint8_t firstTrebleBin,
                              float& fullFlux,
                              float& tempoFlux) {
        uint8_t count = (numBins > MAX_BINS) ? MAX_BINS : numBins;
        if (count == 0) {
            fullFlux = 0.0f;
            tempoFlux = 0.0f;
            return;
        }

        float flux = 0.0f;
        float tFlux = 0.0f;
        float totalWeight = 0.0f;
        float totalTempoWeight = 0.0f;

        for (uint8_t i = 0; i < count; i++) {
            float mag = clampf(bins[i], 0.0f, 1.0f);
            float delta = mag - mPrevMag[i];

            float weight = (i < firstMidBin) ? mBassWeight
                           : (i < firstTrebleBin) ? mMidWeight
                           : mTrebleWeight;
            float tempoWeight = weight;
            if (i < firstMidBin) {
                tempoWeight *= TEMPO_BASS_WEIGHT_SCALE;
            } else if (i < firstTrebleBin) {
                tempoWeight *= TEMPO_MID_WEIGHT_SCALE;
            } else {
                tempoWeight *= TEMPO_TREBLE_WEIGHT_SCALE;
            }

            if (delta > 0.0f) {
                flux += delta * weight;
                tFlux += delta * tempoWeight;
            }
            mPrevMag[i] = mag;
            totalWeight += weight;
            totalTempoWeight += tempoWeight;
        }

        if (totalWeight > 0.0f) {
            flux /= totalWeight;
        }
        if (totalTempoWeight > 0.0f) {
            tFlux /= totalTempoWeight;
        }
        fullFlux = flux;
        tempoFlux = tFlux;
    }

    void pushOdf(float odf) {
        mOdfHistory[mOdfIndex] = odf;
        mOdfIndex = (mOdfIndex + 1) % ODF_HISTORY_SIZE;
        if (mOdfCount < ODF_HISTORY_SIZE) {
            mOdfCount++;
        }
    }

    void pushTempoOdf(float odf) {
        mTempoOdfHistory[mTempoOdfIndex] = odf;
        mTempoOdfIndex = (mTempoOdfIndex + 1) % ODF_HISTORY_SIZE;
        if (mTempoOdfCount < ODF_HISTORY_SIZE) {
            mTempoOdfCount++;
        }
    }

    float computeThreshold() const {
        uint16_t window = (mOdfCount < ODF_THRESH_WINDOW) ? mOdfCount : ODF_THRESH_WINDOW;
        if (window == 0) return mMinOdfThreshold;

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

        float threshold = mean + mThreshStdMult * stddev;
        return (threshold > mMinOdfThreshold) ? threshold : mMinOdfThreshold;
    }

    void updateTempoEstimate() {
        if (mTempoOdfCount < 32 || mFrameDtMs <= 0.0f) {
            return;
        }

        float minPeriodMs = 60000.0f / mMaxBpm;
        float maxPeriodMs = 60000.0f / mMinBpm;

        uint16_t minLag = static_cast<uint16_t>(clampf(minPeriodMs / mFrameDtMs + 0.5f, 1.0f, static_cast<float>(ODF_HISTORY_SIZE - 1)));
        uint16_t maxLag = static_cast<uint16_t>(clampf(maxPeriodMs / mFrameDtMs + 0.5f, 1.0f, static_cast<float>(ODF_HISTORY_SIZE - 1)));

        if (maxLag <= minLag + 2) {
            return;
        }
        if (maxLag >= mTempoOdfCount) {
            maxLag = mTempoOdfCount - 1;
        }

        uint16_t start = (mTempoOdfIndex + ODF_HISTORY_SIZE - mTempoOdfCount) % ODF_HISTORY_SIZE;
        auto odfAt = [&](uint16_t i) -> float {
            return mTempoOdfHistory[(start + i) % ODF_HISTORY_SIZE];
        };

        float bestCorr = -1.0f;
        uint16_t bestLag = 0;
        float sumCorr = 0.0f;
        uint16_t lagCount = 0;

        float mean = 0.0f;
        for (uint16_t i = 0; i < mTempoOdfCount; i++) {
            mean += odfAt(i);
        }
        mean /= static_cast<float>(mTempoOdfCount);

        float var = 0.0f;
        for (uint16_t i = 0; i < mTempoOdfCount; i++) {
            float v = odfAt(i) - mean;
            var += v * v;
        }
        var /= static_cast<float>(mTempoOdfCount);
        float minVar = mMinOdfThreshold * mMinOdfThreshold * TEMPO_MIN_VAR_SCALE;
        if (var < minVar) {
            mTempoConfidence *= 0.95f;
            return;
        }

        float corrValues[ODF_HISTORY_SIZE] = {0.0f};
        for (uint16_t lag = minLag; lag <= maxLag; lag++) {
            float sum = 0.0f;
            uint16_t n = mTempoOdfCount - lag;
            for (uint16_t i = lag; i < mTempoOdfCount; i++) {
                float a = odfAt(i) - mean;
                float b = odfAt(i - lag) - mean;
                sum += a * b;
            }
            float corr = (n > 0) ? (sum / static_cast<float>(n)) : 0.0f;
            if (corr < 0.0f) corr = 0.0f;
            corrValues[lag] = corr;
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

            float peakTarget = bestCorr * TEMPO_LAG_PEAK_FRACTION;
            uint16_t chosenLag = bestLag;
            for (uint16_t lag = minLag; lag <= maxLag; lag++) {
                if (corrValues[lag] >= peakTarget) {
                    // Prefer the longest lag among near-peak candidates
                    // to avoid double-tempo lock.
                    chosenLag = lag;
                }
            }

            // If a slower harmonic still has strong correlation, prefer it.
            // This biases toward the perceptual beat rather than subdivisions.
            float harmonicThresh = bestCorr * 0.72f;
            uint16_t lag3 = static_cast<uint16_t>((bestLag * 3U) / 2U);
            uint16_t lag2 = static_cast<uint16_t>(bestLag * 2U);
            uint16_t preferredLag = chosenLag;
            if (lag3 > bestLag && lag3 <= maxLag && corrValues[lag3] >= harmonicThresh) {
                preferredLag = lag3;
            }
            if (lag2 > bestLag && lag2 <= maxLag && corrValues[lag2] >= harmonicThresh) {
                preferredLag = lag2;
            }
            if (preferredLag > chosenLag) {
                chosenLag = preferredLag;
            }

            float tempoMs = static_cast<float>(chosenLag) * mFrameDtMs;

            if (mTempoMs > 0.0f) {
                float ratio = tempoMs / mTempoMs;
                if (ratio > 1.8f && ratio < 2.2f) {
                    tempoMs *= 0.5f;
                } else if (ratio > 0.45f && ratio < 0.55f) {
                    tempoMs *= 2.0f;
                }
            }

            if (mTempoMs <= 0.0f) {
                mTempoMs = tempoMs;
            } else {
                mTempoMs = mTempoMs * (1.0f - mTempoSmoothAlpha) + tempoMs * mTempoSmoothAlpha;
            }
            mTempoConfidence = mTempoConfidence * 0.80f + confidence * 0.20f;
            clampTempoToRange();
        }
    }

    void recordBeat(uint32_t timestampMs) {
        uint32_t prevBeatTime = mLastBeatTime;
        mLastBeatTime = timestampMs;
        mBeatNumber++;
        mBeatDetected = true;

        if (prevBeatTime > 0) {
            uint32_t interval = timestampMs - prevBeatTime;
            if (interval >= mMinBeatIntervalMs && interval <= mMaxBeatIntervalMs * 4U) {
                float intervalMs = static_cast<float>(interval);
                if (mTempoMs > 0.0f) {
                    float ratio = intervalMs / mTempoMs;
                    float k = std::round(ratio);
                    // Only fold intervals that are close to an integer multiple.
                    // This avoids reinforcing incorrect tempi (e.g. 1.5x).
                    if (k >= 1.0f && k <= 4.0f) {
                        float diff = std::fabs(ratio - k);
                        if (diff <= 0.15f && mTempoConfidence > 0.55f) {
                            float adjusted = intervalMs / k;
                            if (adjusted >= static_cast<float>(mMinBeatIntervalMs) &&
                                adjusted <= static_cast<float>(mMaxBeatIntervalMs)) {
                                intervalMs = adjusted;
                            }
                        }
                    }
                }
                mBeatIntervalCount = (mBeatIntervalCount < 255) ? (mBeatIntervalCount + 1) : 255;
                // Update tempo from recent beat intervals (median-based).
                pushInterval(intervalMs);
                updateTempoFromIntervals();
                if (mTempoMs > 0.0f) {
                    mNextBeatTime = static_cast<float>(timestampMs) + mTempoMs;
                }

                float errFrac = (mTempoMs > 0.0f) ? (std::fabs(intervalMs - mTempoMs) / mTempoMs) : 1.0f;
                float beatConf = clampf(1.0f - (errFrac * 2.0f), 0.0f, 1.0f);
                mTempoConfidence = clampf(mTempoConfidence * 0.8f + beatConf * 0.2f, 0.0f, 1.0f);
                mConfidence = clampf((mTempoConfidence * 0.6f) + (beatConf * 0.4f), 0.0f, 1.0f);
            }
        }
    }

    void updateBpmFromTempo() {
        if (mTempoMs <= 0.0f) {
            mCurrentBPM = 0.0f;
            mSmoothedBPM = 0.0f;
            mConfidence = 0.0f;
            return;
        }
        float bpm = 60000.0f / mTempoMs;
        if (bpm < mMinBpm || bpm > mMaxBpm) {
            mCurrentBPM = 0.0f;
            mSmoothedBPM = 0.0f;
            mConfidence = 0.0f;
            return;
        }

        mCurrentBPM = bpm;
        if (mSmoothedBPM <= 0.0f) {
            mSmoothedBPM = bpm;
        } else {
            mSmoothedBPM += BPM_SMOOTH_ALPHA * (bpm - mSmoothedBPM);
        }
    }

    //=========================================================================
    // State
    //=========================================================================
    float mPrevMag[MAX_BINS] = {0.0f};

    float mOdfHistory[ODF_HISTORY_SIZE] = {0.0f};
    uint16_t mOdfIndex = 0;
    uint16_t mOdfCount = 0;

    float mTempoOdfHistory[ODF_HISTORY_SIZE] = {0.0f};
    uint16_t mTempoOdfIndex = 0;
    uint16_t mTempoOdfCount = 0;

    float mFluxSmooth = 0.0f;
    float mFluxMean = 0.0f;
    float mTempoFluxSmooth = 0.0f;
    float mTempoFluxMean = 0.0f;

    float mOdfPrev2 = 0.0f;
    float mOdfPrev1 = 0.0f;
    float mPrevTimeEnergy = 0.0f;

    uint32_t mLastFrameTime = 0;
    float mFrameDtMs = 0.0f;

    float mTempoMs = 0.0f;
    float mTempoConfidence = 0.0f;
    uint8_t mTempoUpdateCounter = 0;

    uint8_t mBeatIntervalCount = 0;
    float mIntervalHistory[INTERVAL_HISTORY_SIZE] = {0.0f};
    uint8_t mIntervalIndex = 0;
    uint8_t mIntervalCount = 0;

    uint32_t mLastBeatTime = 0;
    uint32_t mBeatNumber = 0;
    bool mBeatDetected = false;
    float mNextBeatTime = 0.0f;

    float mCurrentBPM = 0.0f;
    float mSmoothedBPM = 0.0f;
    float mConfidence = 0.0f;

    float mLastThreshold = 0.0f;
    float mLastOdf = 0.0f;
    float mLastFlux = 0.0f;
    float mLastTempoOdf = 0.0f;

    float mMinBpm = DEFAULT_MIN_BPM;
    float mMaxBpm = DEFAULT_MAX_BPM;
    uint32_t mMinBeatIntervalMs = static_cast<uint32_t>(60000.0f / DEFAULT_MAX_BPM);
    uint32_t mMaxBeatIntervalMs = static_cast<uint32_t>(60000.0f / DEFAULT_MIN_BPM);

    float mBassWeight = DEFAULT_BASS_WEIGHT;
    float mMidWeight = DEFAULT_MID_WEIGHT;
    float mTrebleWeight = DEFAULT_TREBLE_WEIGHT;

    float mOdfSmoothAlpha = DEFAULT_ODF_SMOOTH_ALPHA;
    float mOdfMeanAlpha = DEFAULT_ODF_MEAN_ALPHA;
    float mThreshStdMult = DEFAULT_THRESH_STD_MULT;
    float mMinOdfThreshold = DEFAULT_MIN_ODF_THRESHOLD;

    uint8_t mTempoUpdateInterval = DEFAULT_TEMPO_UPDATE_INTERVAL;
    float mTempoSmoothAlpha = DEFAULT_TEMPO_SMOOTH_ALPHA;

    void resetTempoState() {
        mBeatNumber = 0;
        mLastBeatTime = 0;
        mBeatDetected = false;
        mNextBeatTime = 0.0f;
        mBeatIntervalCount = 0;
        mIntervalIndex = 0;
        mIntervalCount = 0;
        for (uint8_t i = 0; i < INTERVAL_HISTORY_SIZE; i++) {
            mIntervalHistory[i] = 0.0f;
        }

        mTempoMs = 0.0f;
        mTempoConfidence = 0.0f;
        mTempoUpdateCounter = 0;

        mCurrentBPM = 0.0f;
        mSmoothedBPM = 0.0f;
        mConfidence = 0.0f;
    }

    void clampTempoToRange() {
        if (mTempoMs <= 0.0f) return;
        float bpm = 60000.0f / mTempoMs;
        if (bpm <= 0.0f) return;
        while (bpm < mMinBpm) bpm *= 2.0f;
        while (bpm > mMaxBpm) bpm *= 0.5f;
        if (bpm < mMinBpm || bpm > mMaxBpm) {
            return;
        }
        mTempoMs = 60000.0f / bpm;
    }

    void pushInterval(float intervalMs) {
        mIntervalHistory[mIntervalIndex] = intervalMs;
        mIntervalIndex = (mIntervalIndex + 1) % INTERVAL_HISTORY_SIZE;
        if (mIntervalCount < INTERVAL_HISTORY_SIZE) {
            mIntervalCount++;
        }
    }

    float computeMedian(const float* values, uint8_t count) const {
        float temp[INTERVAL_HISTORY_SIZE];
        for (uint8_t i = 0; i < count; i++) {
            temp[i] = values[i];
        }
        for (uint8_t i = 1; i < count; i++) {
            float key = temp[i];
            int j = static_cast<int>(i) - 1;
            while (j >= 0 && temp[j] > key) {
                temp[j + 1] = temp[j];
                j--;
            }
            temp[j + 1] = key;
        }
        if (count == 0) return 0.0f;
        if (count & 1U) {
            return temp[count / 2];
        }
        return 0.5f * (temp[count / 2 - 1] + temp[count / 2]);
    }

    void updateTempoFromIntervals() {
        if (mIntervalCount < 3) {
            return;
        }
        float median = computeMedian(mIntervalHistory, mIntervalCount);
        if (median <= 0.0f) {
            return;
        }

        float mean = 0.0f;
        for (uint8_t i = 0; i < mIntervalCount; i++) {
            mean += mIntervalHistory[i];
        }
        mean /= static_cast<float>(mIntervalCount);

        float var = 0.0f;
        for (uint8_t i = 0; i < mIntervalCount; i++) {
            float d = mIntervalHistory[i] - mean;
            var += d * d;
        }
        var /= static_cast<float>(mIntervalCount);
        float stddev = std::sqrt(var);
        float rel = (median > 0.0f) ? (stddev / median) : 1.0f;
        float intervalConf = clampf(1.0f - (rel / 0.15f), 0.0f, 1.0f);

        if (mTempoMs <= 0.0f) {
            mTempoMs = median;
        } else {
            mTempoMs = mTempoMs * 0.75f + median * 0.25f;
        }
        clampTempoToRange();
        mTempoConfidence = clampf(mTempoConfidence * 0.7f + intervalConf * 0.3f, 0.0f, 1.0f);
    }
};

} // namespace myAudio
