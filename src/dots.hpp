#pragma once

#include "dots_detail.hpp"

namespace dots {
    extern bool dotsInstance;
    
    void initDots(uint16_t (*xy_func)(uint8_t, uint8_t));
    void runDots();
}