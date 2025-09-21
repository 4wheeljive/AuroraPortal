#pragma once

#include "bleControl.h"

#include "fl/ui.h"
#include "fl/audio.h"
#include "fl/fft.h"
#include "fl/xymap.h"
#include "fl/math.h"
#include "fl/math_macros.h"

#include "fl/audio_input.h"
#include "fl/type_traits.h"
//#include "platforms/esp/32/audio/sound_util.h"

namespace audioInput {

    // Audio configuration
    #define SAMPLE_RATE 44100ul
    #define FFT_SIZE 512

    // I2S Configuration
    #define I2S_WS_PIN 5  // Word Select (WS) (YELLOW)
    #define I2S_SD_PIN 8  // Serial Data (SD) (GREEN)
    #define I2S_CLK_PIN 9 // Serial Clock (SCK) (BLUE)
    #define I2S_CHANNEL fl::Right

    fl::AudioConfig config = fl::AudioConfig::CreateInmp441(I2S_WS_PIN, I2S_SD_PIN, I2S_CLK_PIN, I2S_CHANNEL);
    fl::shared_ptr<fl::IAudioInput> audioSource;

    SoundLevelMeter soundMeter(0.0f,0.0f);

    using fl::i16;

    void initAudio() {

        fl::string errorMsg;
        audioSource = fl::IAudioInput::create(config, &errorMsg);

        Serial.println("Waiting 5000ms for audio device to stdout initialization...");
        delay(5000);

        if (!audioSource) {
            Serial.print("Failed to create audio source: ");
            Serial.println(errorMsg.c_str());
            return;
        }

        // Start audio capture
        Serial.println("Starting audio capture...");
        audioSource->start();

        // Check for start errors
        fl::string startErrorMsg;
        if (audioSource->error(&startErrorMsg)) {
            Serial.print("Audio start error: ");
            Serial.println(startErrorMsg.c_str());
            return;
        }

        Serial.println("Audio capture started!");

    } // initAudio

    static const int NUM_BANDS = 32;
    float fftSmooth[NUM_BANDS] = {0};
    float beatHistory[20] = {0};
    int beatHistoryIndex = 0;
    float beatAverage = 0;
    float beatVariance = 0;
    uint32_t lastBeatTime = 0;
    bool isBeat = false;
    float autoGainValue = 1.0f;
    float peakLevel = 0;
    
    // Global audio data accessible to visualizers
    FFTBins fftBins(NUM_BANDS);
    fl::AudioSample currentSample;
    float currentRms = 0.0f;

    // Beat detection algorithm
    bool detectBeat(float energy) {
        beatHistory[beatHistoryIndex] = energy;
        beatHistoryIndex = (beatHistoryIndex + 1) % 20;
        
        // Calculate average
        beatAverage = 0;
        for (int i = 0; i < 20; i++) {
            beatAverage += beatHistory[i];
        }
        beatAverage /= 20.0f;
        
        // Calculate variance
        beatVariance = 0;
        for (int i = 0; i < 20; i++) {
            float diff = beatHistory[i] - beatAverage;
            beatVariance += diff * diff;
        }
        beatVariance /= 20.0f;
        
        // Detect beat
        float threshold = beatAverage + (cBeatSensitivity * sqrt(beatVariance));
        uint32_t currentTime = millis();
        
        if (energy > threshold && (currentTime - lastBeatTime) > 80) {
            lastBeatTime = currentTime;
            return true;
        }
        return false;
    }

    // Update auto gain
    void updateAutoGain(float level) {
        if (!cAutoGain) {
            autoGainValue = 1.0f;
            return;
        }
        
        static float targetLevel = 0.7f;
        static float avgLevel = 0.0f;
        
        avgLevel = avgLevel * 0.95f + level * 0.05f;
        
        if (avgLevel > 0.01f) {
            float gainAdjust = targetLevel / avgLevel;
            gainAdjust = fl::clamp(gainAdjust, 0.5f, 2.0f);
            autoGainValue = autoGainValue * 0.9f + gainAdjust * 0.1f;
        }
    }

    //=============================================================================

    void sampleAudio() {

        // Check if audio is enabled
        if (!cEnableAudio) {
            Serial.println("Audio not enabled!");
            delay(1000);
            return;
        }
                
        // Check if audio source is valid
        if (!audioSource) {
            Serial.println("Audio source is null!");
            delay(1000);
            return;
        }

        // Check for audio errors
        fl::string errorMsg;
        if (audioSource->error(&errorMsg)) {
            Serial.print("Audio error: ");
            Serial.println(errorMsg.c_str());
            delay(100);
            return;
        }
        
        // Check if audioSource is receiving data
        if(debug) {
            static uint32_t lastDebugTime = 0;
            if (millis() - lastDebugTime > 1000) {
                Serial.println("Audio source status: OK, reading samples...");
                lastDebugTime = millis();
            }
        }

        // Read audio data
        currentSample = audioSource->read();

        if(debug) {
            Serial.print("Sample valid: ");
            Serial.print(currentSample.isValid());
            Serial.print(", PCM size: ");
            Serial.println(currentSample.pcm().size());
        }
        
        if (currentSample.isValid()) {
        
            // Update sound meter
            soundMeter.processBlock(currentSample.pcm());
            
            // Get audio levels
            currentRms = currentSample.rms() / 32768.0f;
            if(debug) {
                Serial.print("RMS: ");
                Serial.print(currentRms);
                Serial.print(", Raw samples[0-3]: ");
                Serial.print(currentSample.pcm()[0]);
                Serial.print(", ");
                Serial.print(currentSample.pcm()[1]); 
                Serial.print(", ");
                Serial.print(currentSample.pcm()[2]);
                Serial.print(", ");
                Serial.println(currentSample.pcm()[3]);
            }
            
            // Calculate peak
            int32_t maxSample = 0;
            for (size_t i = 0; i < currentSample.pcm().size(); i++) {
                int32_t absSample = fabsf(currentSample.pcm()[i]);
                if (absSample > maxSample) {
                    maxSample = absSample;
                }
            }
            float peak = float(maxSample) / 32768.0f;
            peakLevel = peakLevel * 0.9f + peak * 0.1f;  // Smooth peak
            if(debug) {
                Serial.print("Peak level: ");
                Serial.println(peakLevel);
            }

            // Update auto gain
            updateAutoGain(currentRms);
            
            // Beat detection
            if (cBeatDetect) {
                isBeat = detectBeat(peak);
            }
            
            // Update global FFT data
            currentSample.fft(&fftBins);
    
        } // if sample.isValid
    
    } // sampleAudio()    

} // namespace audioInput        