#pragma once

namespace myAudio {

     void printDiagnostics() {
        const auto& f = gAudioFrame;
        uint8_t limit = maxBins ? bin32.NUM_FFT_BINS : bin16.NUM_FFT_BINS;
        /*
        FASTLED_DBG("rmsRaw " << (f.rms_raw / 32768.0f)
                               << " rmsSm " << (f.rms / 32768.0f)
                               << " gate " << (noiseGateOpen ? 1 : 0));
        FASTLED_DBG("blockRms(raw) " << (int)lastBlockRms
                               << " openAt " << (int)cNoiseGateOpen
                               << " closeAt " << (int)cNoiseGateClose
                               << " valid " << lastValidSamples << "/" << lastClampedSamples);
        FASTLED_DBG("pcmMin " << lastPcmMin << " pcmMax " << lastPcmMax);
        FASTLED_DBG("autoGain " << (autoGain ? 1 : 0)
                               << " agCeil " << lastAutoGainCeil
                               << " agDesired " << lastAutoGainDesired
                               << " agVal " << autoGainValue
                               << " cAudioGain " << cAudioGain
                               << " gainLevel " << vizConfig.gainLevel);
        FASTLED_DBG("agCeilx1000 " << (lastAutoGainCeil * 1000.0f));
        */
        FASTLED_DBG("rmsNorm " << f.rms_norm);
        //FASTLED_DBG("busA._norm " << f.busA._norm);
        FASTLED_DBG("busB: norm " << f.busB.norm
                    << " normEMA " << f.busB.normEMA
                    << " avResponse " << f.busB.avResponse);
        //FASTLED_DBG("busC._norm " << f.busC._norm);
        FASTLED_DBG("vocals: rawConf " << f.vocalConfidence
                    << "rawEMA " << f.vocalConfidenceEMA);
                    //<< "voxLevel" << voxLevel);
        FASTLED_DBG("---------- ");
       
    }
    
    void printBusSettings () {
        const auto& f = gAudioFrame;
        FASTLED_DBG("busA.thresh " << f.busA.threshold);
        FASTLED_DBG("busA.minBeatInt " << f.busA.minBeatInterval);
        FASTLED_DBG("busA.peakBase " << f.busA.peakBase);
        FASTLED_DBG("busA.attack " << f.busA.rampAttack);
        FASTLED_DBG("busA.decay " << f.busA.rampDecay);
        FASTLED_DBG("busA.expDecay " << f.busA.expDecayFactor);
        FASTLED_DBG("---");
        FASTLED_DBG("busB.thresh " << f.busB.threshold);
        FASTLED_DBG("busB.minBeatInt " << f.busB.minBeatInterval);
        FASTLED_DBG("busB.peakBase " << f.busB.peakBase);
        FASTLED_DBG("busB.attack " << f.busB.rampAttack);
        FASTLED_DBG("busB.decay " << f.busB.rampDecay);
        FASTLED_DBG("busB.expDecay " << f.busB.expDecayFactor);
        FASTLED_DBG("---");
        FASTLED_DBG("busC.thresh " << f.busC.threshold);
        FASTLED_DBG("busC.minBeatInt " << f.busC.minBeatInterval);
        FASTLED_DBG("busC.peakBase " << f.busC.peakBase);
        FASTLED_DBG("busC.attack " << f.busC.rampAttack);
        FASTLED_DBG("busC.decay " << f.busC.rampDecay);
        FASTLED_DBG("busC.expDecay " << f.busC.expDecayFactor);
        FASTLED_DBG("---");
    }

} // namespace myAudio
