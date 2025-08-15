#pragma once

#include <Arduino.h>
#include <FastLED.h>

#include "bubble_detail.hpp"

namespace bubble {

    bubble_detail::BubbleEffect* bubbleInstance = nullptr;

    void initBubble(uint8_t width, uint8_t height, CRGB* leds, uint16_t (*xyFunction)(uint8_t, uint8_t)) {
        if (bubbleInstance) {
            delete bubbleInstance;
        }
        bubbleInstance = new bubble_detail::BubbleEffect(width, height, leds, xyFunction);
    }

    void runBubble() {
        if (bubbleInstance) {
            bubbleInstance->run();
        }
    }
    
    void resetBubble() {
        if (bubbleInstance) {
            bubbleInstance->reset();
        }
    }

    void cleanupBubble() {
        if (bubbleInstance) {
            delete bubbleInstance;
            bubbleInstance = nullptr;
        }
    }

} // namespace bubble
