#pragma once

namespace myAudio {

     // Get RMS from the filtered sample
    // This function just adds temporal smoothing for stability
    float getRMS() {
        if (!filteredSample.isValid()) return 0.0f;

        // Get RMS directly from the pre-filtered sample
        float rms = filteredSample.rms();

        // Temporal smoothing using median filter to reject occasional outliers
        static float history[4] = {0, 0, 0, 0};
        static uint8_t histIdx = 0;

        // Store current value in history
        history[histIdx] = rms;
        histIdx = (histIdx + 1) % 4;

        // Find median of last 4 values (simple sort for 4 elements)
        float sorted[4];
        for (int i = 0; i < 4; i++) sorted[i] = history[i];
        for (int i = 0; i < 3; i++) {
            for (int j = i + 1; j < 4; j++) {
                if (sorted[i] > sorted[j]) {
                    float tmp = sorted[i];
                    sorted[i] = sorted[j];
                    sorted[j] = tmp;
                }
            }
        }
        // Median is average of middle two values
        float median = (sorted[1] + sorted[2]) / 2.0f;

        // Additional smoothing on the median
        static float smoothedRMS = 0.0f;
        if (median > smoothedRMS) {
            // Moderate attack: 70% new, 30% old [was 50/50]
            smoothedRMS = median * 0.7f + smoothedRMS * 0.3f;
        } else {
            // Moderate decay: 70% new, 30% old
            smoothedRMS = median * 0.7f + smoothedRMS * 0.3f;
        }

        return smoothedRMS;
    } // getRMS()
    
    // Get filtered PCM data for waveform visualization
    fl::Slice<const int16_t> getPCM() {
        return filteredSample.pcm();
    }

    // Get raw (unfiltered) PCM data - for diagnostics only
    fl::Slice<const int16_t> getRawPCM() {
        return currentSample.pcm();
    }

    // =====================================================================
    // FFT - Unified FFT path (avoids AudioContext stack usage)
    // Uses static FFT storage and explicit args for consistent bins.
    // =====================================================================

    static fl::FFTBins fftBins(myAudio::MAX_FFT_BINS);
    static fl::FFT fftEngine;

    const fl::FFTBins* getFFT(binConfig& b) {
        if (!filteredSample.isValid()) return nullptr;

        const auto &pcm = filteredSample.pcm();
        if (pcm.size() == 0) return nullptr;

        int sampleRate = fl::FFT_Args::DefaultSampleRate();
        if (config.is<fl::AudioConfigI2S>()) {
            sampleRate = static_cast<int>(config.get<fl::AudioConfigI2S>().mSampleRate);
        } else if (config.is<fl::AudioConfigPdm>()) {
            sampleRate = static_cast<int>(config.get<fl::AudioConfigPdm>().mSampleRate);
        }

        fl::FFT_Args args(
            static_cast<int>(pcm.size()),
            b.NUM_FFT_BINS,
            FFT_MIN_FREQ,
            FFT_MAX_FREQ,
            sampleRate
        );

        fl::span<const fl::i16> span(pcm.data(), pcm.size());
        fftEngine.run(span, &fftBins, args);
        return &fftBins;
    }

    const fl::FFTBins* getFFT_direct(binConfig& b) {
        return getFFT(b);
    }

}