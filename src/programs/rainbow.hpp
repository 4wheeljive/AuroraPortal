#pragma once

//#include "fx/fx2d.h"
#include "rainbow_detail.hpp"


namespace rainbow {
    extern bool rainbowInstance;
    
    void initRainbow(uint16_t (*xy_func)(uint8_t, uint8_t));
    void runRainbow();

    //FASTLED_SMART_PTR(Rainbow);

    /*
    class Rainbow : public Fx2d {
    
          public:

            Rainbow(uint16_t num_leds) : Fx2d(num_leds) {}

            fl::string fxName() const override { return "Rainbow"; }
            void draw(Fx::DrawContext context) override;
    
            void Rainbow::draw(DrawContext ctx) {
                this->leds = ctx.leds;
                
                // need code to elaborate on draw method

                }
                            
                this->leds = nullptr;
            }

        }; // class Rainbow */

}




