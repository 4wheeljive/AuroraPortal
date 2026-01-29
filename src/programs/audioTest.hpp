#pragma once

#include "audioTest_detail.hpp"

namespace audioTest {
    extern bool audioTestInstance;
    
    void initAudioTest(uint16_t (*xy_func)(uint8_t, uint8_t));
    void runAudioTest();
    bool needsFftForMode();

} // namespace audioTest
