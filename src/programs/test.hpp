// https://editor.soulmatelights.com/gallery/2387-pithon


#pragma once

#include "test_detail.hpp"

namespace test {
    extern bool testInstance;
    
    void initTest(uint16_t (*xy_func)(uint8_t, uint8_t));
    void runTest();
}