#pragma once

//#include <FastLED.h>
//#include <fx/fx.h>
#include "waves_detail.hpp"

namespace waves {
    extern bool wavesInstance;
    
    void initWaves();
    void runWaves();

    /*namespace prideWaves {    

        class PrideWaves : public Fx1d {
    
          public:

            PrideWaves(uint16_t num_leds) : Fx1d(num_leds) {}

            fl::string fxName() const override { return "PrideWaves"; }
            void draw(Fx::DrawContext context) override;
    
        }; // class PrideWaves

    } // namespace prideWaves
    */

} // namespace waves