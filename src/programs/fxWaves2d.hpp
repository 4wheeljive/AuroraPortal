#pragma once

#include "fxWaves2d_detail.hpp"

namespace fxWaves2d {
    extern bool fxWaves2dInstance;
    
    void initFxWaves2d(XYMap& myXYmap, XYMap& xyRect);
    void runFxWaves2d();
}