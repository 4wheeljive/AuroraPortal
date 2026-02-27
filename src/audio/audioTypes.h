#pragma once

// =====================================================
// audioTypes.h — All audio type definitions, constants,
// global instances, and type-init functions.
// =====================================================

#include "fl/audio.h"
#include "fl/fft.h"
#include "fl/audio/audio_processor.h"
#include "fl/math_macros.h"

namespace myAudio {

    using namespace fl;

    //=====================================================================
    // Constants
    //=====================================================================

    constexpr uint8_t MAX_FFT_BINS = 32;
    constexpr float FFT_MIN_FREQ = 60.0f;
    constexpr float FFT_MAX_FREQ = 8000.f;
    constexpr uint8_t NUM_BUSES = 3;

    // Scale factors: single user control → domain-specific internal values
    // Level (RMS) values are tiny (~0.001-0.02), FFT/dB values are larger (~0.1-0.8)
    constexpr float FLOOR_SCALE_LEVEL = 0.05f;
    constexpr float FLOOR_SCALE_FFT   = 0.3f;
    constexpr float GAIN_SCALE_LEVEL  = 12.0f;
    constexpr float GAIN_SCALE_FFT    = 8.0f;

    //=====================================================================
    // FFT bin configuration
    //=====================================================================

    struct binConfig {
        uint8_t NUM_FFT_BINS;
    };

    binConfig bin16;
    binConfig bin32;

    void setBinConfig() {
        bin16.NUM_FFT_BINS = 16;
        bin32.NUM_FFT_BINS = 32;
    }

    /* Frequency bin reference (16-bin, log spacing) ------
        f(n) = 60 * (8000/60)^(n/15)
        Bin  Center Hz  Range label
        0    60         sub-bass
        1    83         bass
        2    115        bass
        3    160        bass
        4    221        upper-bass
        5    307        low-mid
        6    425        mid
        7    589        mid
        8    816        upper-mid
        9    1131       upper-mid
        10   1567       presence
        11   2171       presence
        12   3009       high
        13   4170       high
        14   5778       high
        15   8000       high
    ---------------------------------------------------*/

    //=====================================================================
    // Bus — per-frequency-band beat detector + envelope follower
    //=====================================================================

    struct Bus {

        // INPUTS
        float threshold = 0.40f;
        float minBeatInterval = 250.f; //milliseconds
        float expDecayFactor = 0.95f; // for exponential decay
        float rampAttack = 0.0f;
        float rampDecay = 300.f;
        float peakBase = 1.0f;

        // INTERNAL
        uint8_t id = 0;
        bool isActive = false;
        float avgLevel = 0.01f;  // linear scale: fft_pre = bins_raw/32768; typical music ~0.01-0.10
        float energyEMA = 0.0f;
        float normEMA = 0.0f;
        float relativeIncrease = 0.0f;
        uint32_t lastBeat = 0;
        float preNorm = 0.0f;
        float norm = 0.0f;
        float factor = 0.0f;

        // OUTPUTS
        bool newBeat = false;
        float avResponse = 0.0f;
    };

    Bus busA{.id = 0};
    Bus busB{.id = 1};
    Bus busC{.id = 2};

    void initBus(Bus& bus) {
        // Inputs
        bus.threshold = 0.40f;
        bus.minBeatInterval = 250.f;
        bus.expDecayFactor = 0.85f;
        bus.rampAttack = 0.f;
        bus.rampDecay = 300.f;
        bus.peakBase = 1.0f;

        // Output/Internal
        bus.newBeat = false;
        bus.isActive = false;
        bus.avgLevel = 0.01f;
        bus.energyEMA = 0.0f;
        bus.normEMA = 0.0f;
        bus.lastBeat = 0;
        bus.preNorm = 0.0f;
        bus.norm = 0.0f;
        bus.factor = 0.0f;
        bus.avResponse = 0.0f;
    }

    //=====================================================================
    // FFT bin → bus routing
    //=====================================================================

    struct Bin {
        Bus* bus = nullptr;
    };

    Bin bin[MAX_FFT_BINS];

    void initBins() {
        for (uint8_t i = 0; i < MAX_FFT_BINS; i++ ) {
            bin[i].bus = nullptr;
        }

        // target: kick drum
        bin[0].bus = &busA;
        bin[1].bus = &busA;
        bin[2].bus = &busA;
        bin[3].bus = &busA;
        bin[4].bus = &busA;

        // target: snare/mid percussive
        bin[4].bus = &busB;
        bin[5].bus = &busB;
        bin[6].bus = &busB;
        bin[7].bus = &busB;
        bin[8].bus = &busB;
        bin[9].bus = &busB;

        // target: vocals/"lead instruments"
        bin[7].bus = &busC;
        bin[8].bus = &busC;
        bin[9].bus = &busC;
        bin[10].bus = &busC;
        bin[11].bus = &busC;
        bin[12].bus = &busC;
    }

    //=====================================================================
    // AudioVizConfig — visualization normalization & auto-calibration
    //=====================================================================

    struct AudioVizConfig {
        // Internal values (derived from single user controls via scale factors)
        float audioFloorLevel = 0.0f;
        float audioFloorFft = 0.0f;
        float gainLevel = GAIN_SCALE_LEVEL;
        float gainFft = GAIN_SCALE_FFT;

        bool autoGain = true;
        float autoGainTarget = 0.5f;   // P90 ceiling → rms_norm target for loud signals
        bool autoFloor = true;
        float autoFloorAlpha = 0.01f;
        float autoFloorMin = 0.0f;
        float autoFloorMax = 0.5f;
    };

    AudioVizConfig vizConfig;
    float autoGainValue = 1.0f;

    //=====================================================================
    // AudioFrame — per-frame snapshot of all audio data
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

    //=====================================================================
    // Core audio objects
    //=====================================================================

    AudioSample currentSample;      // Raw sample from I2S (kept for diagnostics)
    AudioSample filteredSample;     // Spike-filtered sample for processing
    AudioProcessor audioProcessor;
    bool audioProcessingInitialized = false;

    // Buffer for filtered PCM data
    int16_t filteredPcmBuffer[512];  // Matches I2S_AUDIO_BUFFER_LEN

    //=====================================================================
    // Pipeline state variables
    //=====================================================================

    bool noiseGateOpen = false;
    float lastBlockRms = 0.0f;
    float lastAutoGainCeil = 0.0f;
    float lastAutoGainDesired = 0.0f;
    uint16_t lastValidSamples = 0;
    uint16_t lastClampedSamples = 0;
    int16_t lastPcmMin = 0;
    int16_t lastPcmMax = 0;
    
    //=====================================================================
    // Vocal detection state (shared between pipeline and avHelpers)
    //=====================================================================

    float voxConf = 0.f;
    float voxConfEMA = 0.0f;
    float smoothedVoxConf = 0.0f;
    float scaledVoxConf = 0.0f;
    float voxApprox = 0.0f;
    float voxApproxEMA = 0.0f;

} // namespace myAudio
