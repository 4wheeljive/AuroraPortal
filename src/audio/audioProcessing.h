#pragma once

// =====================================================
// THIS IS EARLY-DEVELOPMENT EXPERIMENTAL STUFF
// USE AT YOUR OWN RISK! ;-)
// =====================================================

#include "fl/audio.h"
#include "fl/fft.h"
//#include "fl/audio/audio_context.h"
#include "fl/fx/audio/audio_processor.h"
#include "fl/math_macros.h"
#include "bleControl.h"
#include "audioInput.h"

// audioBins.h is self-contained (only standard types + a Bus forward decl).
// It defines MAX_FFT_BINS, binConfig, Bin, and bin[] — needed by the first
// namespace block and all downstream files.
#include "audioBins.h"

//=========================================================================
// NAMESPACE BLOCK 1 — Core audio objects and state variables.
// All downstream headers (audioGainControl, audioAnalysis, audioSample,
// audioBus → audioFrame) see everything declared here via namespace lookup.
//=========================================================================

namespace myAudio {

    using namespace fl;

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
    // State variables (populated by sampleAudio / callbacks)
    //=========================================================================

    bool noiseGateOpen = false;
    float lastBlockRms = 0.0f;
    float lastAutoGainCeil = 0.0f;
    float lastAutoGainDesired = 0.0f;
    uint16_t lastValidSamples = 0;
    uint16_t lastClampedSamples = 0;
    int16_t lastPcmMin = 0;
    int16_t lastPcmMax = 0;
    bool vocalsActive = false;

} // namespace myAudio (first block)

//=========================================================================
// Processing modules — included here so their inline functions see the
// core variables declared above.  Order matters: each file can only use
// symbols defined by files earlier in this list.
//=========================================================================

#include "audioGainControl.h"   // AudioVizConfig, vizConfig, updateVizConfig/AutoGain/AutoFloor
                                 //   needs: noiseGateOpen, lastAutoGainCeil, lastAutoGainDesired,
                                 //          cAudioFloor, cAudioGain, autoGain, autoFloor (bleControl)

#include "audioAnalysis.h"      // getRMS, getPCM, getRawPCM, getFFT, getFFT_direct
                                 //   needs: filteredSample, currentSample, binConfig, config

#include "audioSample.h"        // sampleAudio
                                 //   needs: currentSample, filteredSample, filteredPcmBuffer,
                                 //          noiseGateOpen, audioProcessor, audioSource, etc.

#include "audioBus.h"           // Bus, busA/B/C, initBus, initBins, updateBus, finalizeBus
                                 //   → audioBus.h internally #includes audioFrame.h after
                                 //     defining Bus, so AudioFrame and captureAudioFrame are
                                 //     also pulled in here.
                                 //   audioFrame.h needs: sampleAudio ✓, updateVizConfig ✓,
                                 //     getRMS/getFFT ✓, noiseGateOpen/busA-C/filteredSample ✓

//=========================================================================
// NAMESPACE BLOCK 2 — Pipeline management: frame cache, init, diagnostics.
// AudioFrame is now complete (pulled in via audioBus.h → audioFrame.h).
//=========================================================================

namespace myAudio {

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
        } // if (audioLatencyDiagnostics)

        gAudioFrame = frame;
        gAudioFrameInitialized = true;
        gAudioFrameLastMs = now;

        return gAudioFrame;
    }

    inline const AudioFrame& getAudioFrame() {
        return gAudioFrame;
    }

    //=========================================================================
    // Bus parameter handler (called from BLE callbacks)
    //=========================================================================

    static void handleBusParam(uint8_t busId, const String& paramId, float value) {
        Bus* bus = (busId == 0) ? &busA : (busId == 1) ? &busB : &busC;
        if (!bus) return;
        if      (paramId == "inThreshold")      bus->threshold = value;
        else if (paramId == "minBeatInterval")  bus->minBeatInterval = value;
        else if (paramId == "inExpDecayFactor") bus->expDecayFactor = value;
        else if (paramId == "inRampAttack")     bus->rampAttack = value;
        else if (paramId == "inRampDecay")      bus->rampDecay = value;
        else if (paramId == "inPeakBase")       bus->peakBase = value;
    }

    //=========================================================================
    // Initialize audio processing
    //=========================================================================

    void initAudioProcessing() {

        if (audioProcessingInitialized) { return; }

        // Initialize bin/bus configurations
        setBinConfig();
        initBus(busA);
        initBus(busB);
        initBus(busC);
        initBins();
        setBusParam = handleBusParam;

        audioProcessor.onVocalStart([]() {
            vocalsActive = true;
        });

        audioProcessor.onVocalEnd([]() {
            vocalsActive = false;
        });

        Serial.println("AudioProcessor initialized with callbacks");
        audioProcessingInitialized = true;
    }

} // namespace myAudio (second block)

// Debug output — needs gAudioFrame (complete), bin16/bin32, maxBins
#include "audioDebug.h"
