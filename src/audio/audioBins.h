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
    constexpr float FFT_MAX_FREQ = 8000.f;
 
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
        Log spacing: f(n) = 60 * (8000/60)^(n/15)
        Bin	Center Hz	Range label
        0	60	    	sub-bass
        1	83	    	bass
        2	115	    	bass
        3	160	    	bass
        4	221		    upper-bass
        5	307	    	low-mid
        6	425	    	mid
        7	589	    	mid
        8	816		    upper-mid
        9	1131		upper-mid
        10	1567	    presence
        11	2171	    presence
        12	3009	    high
        13	4170		high
        14	5778		high
        15	8000	    high
    ---------------------------------------------------*/

    // initBins() is declared here and defined in audioBus.h,
    // where busA/B/C are available.
    void initBins();

}