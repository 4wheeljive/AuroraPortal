#pragma once

#include "crgb.h"
#include "fl/xymap.h"
#include "fl/stl/scoped_ptr.h"
#include "eorder.h"
#include "pixel_controller.h"

#define ANIMARTRIX_INTERNAL
#include "animartrix_detail.hpp"

namespace animartrix {
    extern bool animartrixInstance;

    void initAnimartrix(const fl::XYMap &xyMap);
    void runAnimartrix();
}
