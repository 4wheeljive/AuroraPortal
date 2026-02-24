#pragma once

#include "fl/time_alpha.h"

namespace myAudio {

float vocalConfidence = 0;
float vocalConfidenceEMA = 0;
float smoothedVocalConfidence = 0;
float voxLevel = 0;


    // basicPulse: avResponse decays exponentially from 1.0
    void basicPulse(Bus& bus){
        if (bus.newBeat) { bus.avResponse = 1.0f;}
        if (bus.avResponse > .1f) {
            bus.avResponse = bus.avResponse * bus.expDecayFactor;  // Exponential decay
        } else {
            bus.avResponse = 0.f;
        }
    }

    // dynamicPulse: louder beat = higher initial avResponse and slower decay
    /*void dynamicPulse(Bus& bus, uint32_t now) {
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
            float t    = intensity / (intensity + 2.0f);                        // [0, 1)
            float ease = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);           // easeOutCubic
            peak = bus.peakBase + ease;                                                 // [1.0, 2.0)
            uint32_t fallingTime = (uint32_t)(30.0f + ease * bus.rampDecay);    // 30~1000 ms
            ramp = fl::TimeRamp(0, 0, fallingTime);
            ramp.trigger(now);
        }

        uint8_t currentAlpha = ramp.update8(now);
        bus.avResponse = peak * currentAlpha / 255.0f;
    }*/


    // dynamicPulse: louder beat = faster rise, higher peak and longer fallTime
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
            float t    = intensity / (intensity + 2.0f);                        // [0, 1)
            float ease = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);           // easeOutCubic
            peak = bus.peakBase + ease * 0.4f;            // bus.peakBase     // ~[0.0, 2.0)
            uint32_t risingTime = (uint32_t)(ease * bus.rampAttack);           // bus.rampAttack
            uint32_t fallingTime = (uint32_t)(30.0f + ease * bus.rampDecay);   // bus.rampDecay
            ramp = fl::TimeRamp(0, risingTime, fallingTime);
            ramp.trigger(now);
        }

        uint8_t currentAlpha = ramp.update8(now);
        bus.avResponse = peak * currentAlpha / 255.0f;
    }


    void ehancedTrend(Bus& bus, uint32_t now) {
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
            float t    = intensity / (intensity + 2.0f);                        // [0, 1)
            float ease = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);           // easeOutCubic
            peak = bus.peakBase + ease * 0.4f;            // bus.peakBase     // ~[0.0, 2.0)
            uint32_t risingTime = (uint32_t)(ease * bus.rampAttack);           // bus.rampAttack
            uint32_t fallingTime = (uint32_t)(30.0f + ease * bus.rampDecay);   // bus.rampDecay
            ramp = fl::TimeRamp(0, risingTime, fallingTime);
            ramp.trigger(now);
        }

        uint8_t currentAlpha = ramp.update8(now);
        bus.avResponse = bus.energyEMA + peak * currentAlpha / 255.0f;    

    }


    // normEnvelope: avResponse tracks normalized energy with fast attack / slow release.
    // Rises quickly on spikes, decays smoothly — no beat trigger required.
    void normEnvelope(Bus& bus) {
        bus.avResponse = bus.normEMA;
    }

    
    float smoothVocalConfidence(float level) {
        constexpr float attack  = 0.35f;  // 0.35 fast rise on spikes
        constexpr float release = 0.20f;  // 0.04f = slow decay
        float alpha  = (level > vocalConfidenceEMA) ? attack : release;
        vocalConfidenceEMA += alpha * (level - vocalConfidenceEMA);
        return vocalConfidenceEMA;
    }

    float vocalResponse() {
        smoothedVocalConfidence = smoothVocalConfidence(cVocalConfidence);
        voxLevel = fl::map_range_clamped<float, float>(smoothedVocalConfidence, 0.f, .75f, 0.f, 1.0f);
        return voxLevel;
    }


} // myAudio
