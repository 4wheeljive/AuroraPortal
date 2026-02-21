#pragma once

// Forward declare Bus so struct Bin can hold a Bus pointer.
// Full Bus definition is in audioBus.h, which includes this file.
namespace myAudio { struct Bus; }

namespace myAudio {

    // FFT configuration
    // binConfig enables runtime switching of bin count/resolution
    //=========================================================================

    // Maximum FFT bins (compile-time constant for array sizing)
    constexpr uint8_t MAX_FFT_BINS = 32;

    constexpr float FFT_MIN_FREQ = 60.0f;    
    constexpr float FFT_MAX_FREQ = 16000.f;
 
    struct binConfig {
        uint8_t NUM_FFT_BINS;
    };

    binConfig bin16;
    binConfig bin32;

    void setBinConfig() {
        bin16.NUM_FFT_BINS = 16;
        bin32.NUM_FFT_BINS = 32;
    }

    struct Bin {
        // Single-bus routing; nullptr = unassigned
        Bus* bus = nullptr;
    };

    Bin bin[MAX_FFT_BINS];

    /* Frequency bin reference ------------------------
        Bin	Center Hz	Range label
        0	60	    	sub-bass 
        1	87	    	bass
        2	126	    	bass
        3	183	    	bass
        4	266		    upper-bass 
        5	386	    	low-mid 
        6	561	    	mid 
        7	813	    	mid 
        8	1180		upper-mid 
        9	1713		upper-mid
        10	2486	    presence 
        11	3607	    presence 
        12	5235	    high 
        13	7597		high
        14	11025		air 
        15	16000	    air
    ---------------------------------------------------*/    

    // initBins() is declared here and defined in audioBus.h,
    // where busA/B/C are available.
    void initBins();

}