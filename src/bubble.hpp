#pragma once

#include "bubble_detail.hpp"

namespace bubble {
    extern bool bubbleInstance;
    
    void initBubble(uint8_t width, uint8_t height, CRGB* leds, uint16_t (*xy_func)(uint8_t, uint8_t));
    void runBubble();
}
