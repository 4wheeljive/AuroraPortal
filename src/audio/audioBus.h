#pragma once

#include "audioBins.h"  // MAX_FFT_BINS, binConfig, Bin, bin[] — must come first

namespace myAudio {

    static constexpr uint8_t NUM_BUSES = 3;

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

    // initBins is declared in audioBins.h; defined here where busA/B/C are available.
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

} // namespace myAudio

// ─────────────────────────────────────────────────────────────────────────────
// updateBus / finalizeBus operate on Bus objects using data from an AudioFrame.
// AudioFrame (in audioFrame.h) uses Bus as a member type, so we have a mutual
// dependency.  We break it with the "include from middle" pattern:
//   1. Bus is fully defined above.
//   2. We forward-declare AudioFrame and declare the two functions so that
//      captureAudioFrame() (inside audioFrame.h, included next) can call them.
//   3. We include audioFrame.h — Bus is complete, so AudioFrame can embed it.
//   4. After the include, AudioFrame is complete, so we can define the functions.
// ─────────────────────────────────────────────────────────────────────────────

namespace myAudio {
    struct AudioFrame;  // forward declaration — full definition follows via audioFrame.h
    void updateBus(const AudioFrame& frame, const binConfig& b, Bus& bus);
    void finalizeBus(const AudioFrame& frame, Bus& bus, float crossCalRatio, float gainApplied);
}

// audioFrame.h must NOT #include "audioBus.h" — it relies on Bus already being
// defined by the time it is included from here.
#include "audioFrame.h"

namespace myAudio {

    inline void updateBus(const AudioFrame& frame, const binConfig& b, Bus& bus) {
        bus.isActive = false;
        bus.newBeat = false;

        if (!frame.valid || !frame.fft_norm_valid) {
            bus.norm = 0.0f;
            bus.factor = 0.0f;
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

        if (count > 0) { bus.isActive = true; }

        float avg = (count > 0) ? (sum / static_cast<float>(count)) : 0.0f;

        constexpr float eqAlpha = 0.02f;  // ~1-2 sec half-life
        bus.avgLevel += eqAlpha * (avg - bus.avgLevel);
        bus.avgLevel = FL_MAX(bus.avgLevel, 0.001f);  // linear scale floor

        // Store spectrally-flattened value (cross-cal and gain applied later)
        bus.norm = avg / bus.avgLevel;
    }

    inline void finalizeBus(const AudioFrame& frame, Bus& bus, float crossCalRatio, float gainApplied) {
        // Capture pre-finalize norm (spectrally-flattened, before cross-cal/gain)
        bus.preNorm = bus.norm;

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

        // --- Beat detection on pre-finalize norm so onset shape isn't distorted ---
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
        bus.norm = fl::clamp(bus.norm * crossCalRatio * gainApplied, 0.0f, 1.0f);

        constexpr float gamma = 0.5754f; // ln(0.5)/ln(0.3)
        bus.factor = 2.0f * fl::powf(bus.norm, gamma);

        // --- Asymmetric EMA of normalized value (envelope follower) ---
        constexpr float normAttack  = 0.4f;  // 0.35 fast rise on spikes
        constexpr float normRelease = 0.3f;  // 0.04f = slow decay
        float normAlpha = (bus.norm > bus.normEMA) ? normAttack : normRelease;
        bus.normEMA += normAlpha * (bus.norm - bus.normEMA);
    }

} // namespace myAudio
