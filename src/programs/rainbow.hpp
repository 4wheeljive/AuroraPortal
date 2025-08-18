#pragma once

//#include "fx/fx2d.h"
#include "rainbow_detail.hpp"


namespace rainbow {
    extern bool rainbowInstance;
    
    void initRainbow(uint16_t (*xy_func)(uint8_t, uint8_t));
    void runRainbow();

    /*
    class Rainbow : public Fx2d {
    
          public:

            Rainbow(uint16_t num_leds) : Fx2d(num_leds) {}

            fl::string fxName() const override { return "Rainbow"; }
            void draw(Fx::DrawContext context) override;
    
            void Rainbow::draw(DrawContext ctx) {
                this->leds = ctx.leds;
                AnimartrixLoop(*this, ctx.now);
                if (color_order != RGB) {
                    for (int i = 0; i < mXyMap.getTotal(); ++i) {
                        CRGB &pixel = ctx.leds[i];
                        const uint8_t b0_index = RGB_BYTE0(color_order);
                        const uint8_t b1_index = RGB_BYTE1(color_order);
                        const uint8_t b2_index = RGB_BYTE2(color_order);
                        pixel = CRGB(pixel.raw[b0_index], pixel.raw[b1_index],
                                    pixel.raw[b2_index]);
                    }

                }
                this->leds = nullptr;
            }

        }; // class Rainbow */

}




