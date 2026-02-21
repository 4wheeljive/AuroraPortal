#pragma once

namespace myAudio {

    void sampleAudio() {

        if (!audioSource) {
            currentSample = AudioSample();
            filteredSample = AudioSample();
            return;
        }

        checkAudioInput();

        fl::string errorMsg;
        if (audioSource->error(&errorMsg)) {
            Serial.print("Audio error: ");
            Serial.println(errorMsg.c_str());
            currentSample = AudioSample();
            filteredSample = AudioSample();
            return;
        }

        // ACTUAL AUDIO SAMPLE CAPTURE
        // Drain all available audio buffers and keep only the newest sample.
        // This prevents accumulating stale audio when render FPS is slower
        // than the audio production rate.
        fl::vector_inlined<AudioSample, 16> samples;
        samples.clear();
        size_t readCount = audioSource->readAll(&samples);
        if (readCount == 0) {
            currentSample = AudioSample();
            filteredSample = AudioSample();
            return;
        }
        currentSample = samples[readCount - 1];

        // DIAGNOSTIC: Check sample validity and raw data
        static uint32_t sampleCount = 0;
        static uint32_t validCount = 0;
        static uint32_t invalidCount = 0;
        sampleCount++;

        if (!currentSample.isValid()) {
            invalidCount++;
            EVERY_N_MILLISECONDS(500) {
                Serial.print("[DIAG] Sample INVALID. Total: ");
                Serial.print(sampleCount);
                Serial.print(" Valid: ");
                Serial.print(validCount);
                Serial.print(" Invalid: ");
                Serial.println(invalidCount);
            }
            return;
        }

        validCount++;

        //=====================================================================
        // SPIKE FILTERING - Filter raw samples BEFORE AudioProcessor
        // This ensures beat detection, FFT, bass/mid/treble all get clean data
        //=====================================================================

        // Spike filtering: I2S occasionally produces spurious samples near int16_t max/min
        constexpr int16_t SPIKE_THRESHOLD = 10000;  // Samples beyond this are glitches

        auto rawPcm = currentSample.pcm();
        size_t n = rawPcm.size();
        size_t nClamped = (n > 512) ? 512 : n;

        /* USED FOR DIAGNOSTICS
        int16_t pcmMin = 32767;
        int16_t pcmMax = -32768;
        for (size_t i = 0; i < nClamped; i++) {
            int16_t v = rawPcm[i];
            if (v < pcmMin) pcmMin = v;
            if (v > pcmMax) pcmMax = v;
        }
        if (nClamped == 0) {
            pcmMin = 0;
            pcmMax = 0;
        }
        lastPcmMin = pcmMin;
        lastPcmMax = pcmMax;
        */
        
        // Calculate DC offset from non-spike samples
        int64_t dcSum = 0;
        size_t dcCount = 0;
        for (size_t i = 0; i < n; i++) {
            if (rawPcm[i] > -SPIKE_THRESHOLD && rawPcm[i] < SPIKE_THRESHOLD) {
                dcSum += rawPcm[i];
                dcCount++;
            }
        }
        int16_t dcOffset = (dcCount > 0) ? static_cast<int16_t>(dcSum / dcCount) : 0;

        // Copy samples to filtered buffer, replacing spikes with zero
        // Also calculate RMS for noise gate decision
        uint64_t sumSq = 0;
        size_t validSamples = 0;

        for (size_t i = 0; i < nClamped; i++) {
            if (rawPcm[i] > -SPIKE_THRESHOLD && rawPcm[i] < SPIKE_THRESHOLD) {
                // Valid sample - keep it (with DC correction applied)
                int16_t corrected = rawPcm[i] - dcOffset;
                filteredPcmBuffer[i] = corrected;
                sumSq += static_cast<int32_t>(corrected) * corrected;
                validSamples++;
            } else {
                // Spike detected - replace with zero
                filteredPcmBuffer[i] = 0;
            }
        }

        // Calculate RMS of the filtered signal
        float blockRMS = (validSamples > 0) ? fl::sqrtf(static_cast<float>(sumSq) / validSamples) : 0.0f;
        lastBlockRms = blockRMS;
        lastValidSamples = static_cast<uint16_t>(validSamples);
        lastClampedSamples = static_cast<uint16_t>(nClamped);

        // EMA-smooth blockRMS before gate decision so brief noise spikes don't open the gate.
        // A single spike to 70 barely moves the EMA; sustained music builds it up within ~10 frames.
        static float blockRmsEMA = 0.0f;
        constexpr float rmsEmaAlpha = 0.15f;
        blockRmsEMA = blockRmsEMA * (1.0f - rmsEmaAlpha) + blockRMS * rmsEmaAlpha;
        lastBlockRms = blockRmsEMA;  // update diagnostic to show smoothed value

        // NOISE GATE with hysteresis to prevent flickering
        // Gate opens when smoothed signal exceeds cNoiseGateOpen
        // Gate closes when smoothed signal falls below cNoiseGateClose
        if (blockRmsEMA >= cNoiseGateOpen) {
            noiseGateOpen = true;
        } else if (blockRmsEMA < cNoiseGateClose) {
            noiseGateOpen = false;
        }
        // Between CLOSE and OPEN thresholds, gate maintains its previous state

        // If gate is closed, zero out entire buffer
        if (!noiseGateOpen) {
            for (size_t i = 0; i < nClamped; i++) {
                filteredPcmBuffer[i] = 0;
            }
        }

        // Create filtered AudioSample from the cleaned buffer
        fl::span<const int16_t> filteredSpan(filteredPcmBuffer, nClamped);
        filteredSample = AudioSample(filteredSpan, currentSample.timestamp());

        // Process through AudioProcessor (triggers callbacks)
            // - only needed if there are active callbacks to trigger
        audioProcessor.update(filteredSample);  // filtered

    } // sampleAudio()

}