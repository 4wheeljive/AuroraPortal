#pragma once

#include "horizons_detail.hpp"

namespace horizons {
    extern bool horizonsInstance;
    
    void initHorizons(uint16_t (*xy_func)(uint8_t, uint8_t));
    void runHorizons();
}
