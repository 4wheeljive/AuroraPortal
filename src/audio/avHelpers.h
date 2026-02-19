#pragma once

#include "bleControl.h"
#include "audio/audioProcessing.h"
#include "fl/time_alpha.h"

namespace myAudio {

    // basicPulse: beatBrightness decays exponentially from 1.0
    void basicPulse(Bus& bus){
        if (bus.newBeat) { bus.beatBrightness = 1.0f;}
        if (bus.beatBrightness > .1f) {
            bus.beatBrightness = bus.beatBrightness * bus.expDecayFactor;  // Exponential decay
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
            float intensity = fl::clamp(bus.relativeIncrease - bus.threshold, 0.0f, 100.0f);
            // Soft saturation: hyperbolic pre-normalize to [0,1), then easeOutCubic.
            // k=2 → 50% saturation at intensity=2; good dynamic range up to ~1.0,
            // barely noticeable above ~5. Both peak and fallingTime use the same
            // eased value so louder beats are brighter AND last longer.
            //
            // Replace cRampFallTime with bus.rampFallTime once bus UI is built out
            float t    = intensity / (intensity + 2.0f);                        // [0, 1)
            float ease = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);           // easeOutCubic
            peak = cPeakBase + ease;                                                 // [1.0, 2.0)
            uint32_t fallingTime = (uint32_t)(30.0f + ease * cRampDecay);    // 30~1000 ms
            ramp = fl::TimeRamp(0, 0, fallingTime);
            ramp.trigger(now);
        }

        uint8_t currentAlpha = ramp.update8(now);
        bus.beatBrightness = peak * currentAlpha / 255.0f;
    }


    // dynamicSwell: louder beat = faster rise, higher peak and longer fallTime
    void dynamicSwell(Bus& bus, uint32_t now, float peakBase, float attack, float decay) {
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
            float intensity = fl::clamp(bus.relativeIncrease - bus.threshold, 0.0f, 100.0f);
            // Soft saturation: hyperbolic pre-normalize to [0,1), then easeOutCubic.
            // k=2 → 50% saturation at intensity=2; good dynamic range up to ~1.0,
            // barely noticeable above ~5. Both peak and fallingTime use the same
            // eased value so louder beats are brighter AND last longer.
            //
            // Replace cRampAttack/Decay & PeakBase with bus. elements once bus UI is built out
            float t    = intensity / (intensity + 2.0f);                        // [0, 1)
            float ease = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);           // easeOutCubic
            peak = cPeakBase + ease;                                            // ~[0.0, 2.0)
            
            uint32_t risingTime = (uint32_t)( ease * cRampAttack);          // 0~500 ms
            uint32_t fallingTime = (uint32_t)(30.0f + ease * cRampDecay);    // 30~1000 ms
            
            ramp = fl::TimeRamp(0, risingTime, fallingTime);
            ramp.trigger(now);
        }

        uint8_t currentAlpha = ramp.update8(now);
        bus.beatBrightness = peak * currentAlpha / 255.0f;
    }

} // myAudio
