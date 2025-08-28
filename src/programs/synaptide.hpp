#pragma once

#include "synaptide_detail.hpp"

namespace synaptide {
    extern bool synaptideInstance;
    
    void initSynaptide(uint16_t (*xy_func)(uint8_t, uint8_t));
    void runSynaptide();
}