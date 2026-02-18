#pragma once

#include "bleControl.h"
#include "audio/audioProcessing.h"
#include "fl/time_alpha.h"

namespace myAudio {

    // basicPulse: beatBrightness decays exponentially from 1.0
    void basicPulse(Bus& bus){
        if (bus.newBeat) { bus.beatBrightness = 1.0f;}
        if (bus.beatBrightness > .1f) {
            bus.beatBrightness = bus.beatBrightness * bus.beatBrightnessDecay;  // Exponential decay
        } else {
            bus.beatBrightness = 0.f;
        }
    }

    // dynamicPulse: louder beat = higher initial beatBrightness and slower decay
    void dynamicPulse(Bus& bus, uint32_t now) {
        // Each bus gets its own TimeRamp instance (static = persists across calls)
        static fl::TimeRamp ramps[NUM_BUSES] = {
            fl::TimeRamp(0, 0, 0),
            fl::TimeRamp(0, 0, 0),
            fl::TimeRamp(0, 0, 0)
        };
        static float peaks[NUM_BUSES] = {0.0f, 0.0f, 0.0f};
        fl::TimeRamp& ramp = ramps[bus.id];
        float& peak = peaks[bus.id];

        if (bus.newBeat) {
            float intensity = bus.relativeIncrease - bus.threshold;
            peak = 1.0f + intensity;
            uint16_t intensity16 = intensity * 32768;
            uint32_t holdTime = map(intensity16, 0, 32768, 10, 50);
            uint32_t fallingTime = map(intensity16, 0, 32768, 100, 1000);
            ramp = fl::TimeRamp(0, holdTime, fallingTime);
            ramp.trigger(now);
        }

        uint8_t currentAlpha = ramp.update8(now);
        bus.beatBrightness = peak * currentAlpha / 255.0f;
    }

} // myAudio
