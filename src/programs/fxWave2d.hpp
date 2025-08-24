#pragma once

#include "fxWave2d_detail.hpp"

namespace fxWave2d {
    extern bool fxWave2dInstance;
    
    void initFxWave2d(XYMap& myXYmap, XYMap& xyRect);
    void runFxWave2d();
}