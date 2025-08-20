#pragma once

#include "bounce_detail.hpp"

namespace bounce {
    extern bool bounceInstance;
    
    void initBounce(uint16_t (*xy_func)(uint8_t, uint8_t), XYMap (*myXYmap));
    void runBounce();
}