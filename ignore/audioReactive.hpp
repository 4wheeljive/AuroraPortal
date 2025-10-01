#pragma once

#include "audioReactive_detail.hpp"

namespace audioReactive {
    extern bool audioReactiveInstance;
    
    void initAudioReactive(uint16_t (*xy_func)(uint8_t, uint8_t));
    void runAudioReactive();

} // namespace audioReactive