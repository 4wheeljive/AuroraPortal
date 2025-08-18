#pragma once

#include "radii_detail.hpp"

namespace radii {
    extern bool radiiInstance;
    
    void initRadii(uint16_t (*xy_func)(uint8_t, uint8_t));
    void runRadii();
}