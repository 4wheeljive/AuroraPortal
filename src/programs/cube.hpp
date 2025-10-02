#pragma once

#include "cube_detail.hpp"

namespace cube {
    extern bool cubeInstance;
    
    void initCube(uint16_t (*xy_func)(uint8_t, uint8_t));
    void runCube();

} // namespace cube