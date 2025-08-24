#pragma once

/*
My personal learning playground for working with:
    FastLED Adapter for the animartrix fx library.
    Copyright Stefan Petrick 2023.
    Adapted to C++ by Netmindz 2023.
    Adapted to FastLED by Zach Vorhies 2024.
    For details on the animartrix library and licensing information, 
    see https://github.com/FastLED/FastLED/blob/master/src/fx/2d/animartrix_detail.hpp
*/

#include "crgb.h"
#include "fl/dbg.h"
#include "fl/namespace.h"
#include "fl/ptr.h"
#include "fl/scoped_ptr.h"
#include "fl/xymap.h"
#include "fx/fx2d.h"
#include "eorder.h"
#include "pixel_controller.h"

#define ANIMARTRIX_INTERNAL
#include "animartrix_detail.hpp"

namespace fl {

    FASTLED_SMART_PTR(Animartrix);

    enum AnimartrixAnim {
        POLAR_WAVES=0,
        SPIRALUS,
        CALEIDO1,
        WAVES,
        CHASING_SPIRALS,
        COMPLEX_KALEIDO_6,
        WATER,
        EXPERIMENT1,
        EXPERIMENT2,
        TEST,
        NUM_ANIMATIONS
    };

    fl::string getAnimartrixName(int animation);

    class FastLEDANIMartRIX;
    
    class Animartrix : public Fx2d {
    
        public:
            Animartrix(XYMap xyMap, AnimartrixAnim which_animation) : Fx2d(xyMap) {
                // Note: Swapping out height and width.
                this->current_animation = which_animation;
                mXyMap.convertToLookUpTable();
            }

            Animartrix(const Animartrix &) = delete;
            void draw(DrawContext context) override;
            //int fxNum() const { return NUM_ANIMATIONS; }
            void fxSet(int fx);
            int fxGet() const { return static_cast<int>(current_animation); }
            Str fxName() const override { return "Animartrix:"; }
            void fxNext(int fx = 1) { fxSet(fxGet() + fx); }
            void setColorOrder(EOrder order) { color_order = order; }
            EOrder getColorOrder() const { return color_order; }

        private:
            friend void AnimartrixLoop(Animartrix &self, uint32_t now);
            friend class FastLEDANIMartRIX;
            static const char *getAnimartrixName(AnimartrixAnim animation);
            AnimartrixAnim prev_animation = NUM_ANIMATIONS;
            fl::scoped_ptr<FastLEDANIMartRIX> impl;
            CRGB *leds = nullptr; // Only set during draw, then unset back to nullptr.
            AnimartrixAnim current_animation = CHASING_SPIRALS;
            EOrder color_order = RGB;

    };

    void AnimartrixLoop(Animartrix &self, uint32_t now);

    //************************************************************************************

    struct AnimartrixEntry {
        AnimartrixAnim anim;
        const char *name;
        void (FastLEDANIMartRIX::*func)();
    };

    class FastLEDANIMartRIX : public animartrix_detail::ANIMartRIX {
        Animartrix *data = nullptr;

        public:
            FastLEDANIMartRIX(Animartrix *_data) {
                this->data = _data;
                this->init(data->getWidth(), data->getHeight());
            }

            void setPixelColor(int x, int y, CRGB pixel) {
                data->leds[xyMap(x, y)] = pixel;
            }
            void setPixelColorInternal(int x, int y,
                                    animartrix_detail::rgb pixel) override {
                setPixelColor(x, y, CRGB(pixel.red, pixel.green, pixel.blue));
            }

            uint16_t xyMap(uint16_t x, uint16_t y) override {
                return data->xyMap(x, y);
            }

            void loop();
    
        };

    //******************************************************************
    
    void Animartrix::fxSet(int fx) {

        int curr = fxGet();
        if (fx < 0) {
            fx = curr + fx;
            if (fx < 0) {
                fx = NUM_ANIMATIONS - 1;
            }
        }
        fx = fx % NUM_ANIMATIONS;
        current_animation = static_cast<AnimartrixAnim>(fx);
        FASTLED_DBG("Setting animation to " << getAnimartrixName(current_animation));

    }

    //******************************************************************
    
    void AnimartrixLoop(Animartrix &self, uint32_t now) {
        if (self.prev_animation != self.current_animation) {
            if (self.impl) {
                // Re-initialize object.
                self.impl->init(self.getWidth(), self.getHeight());
            }
            self.prev_animation = self.current_animation;
        }
        if (!self.impl) {
            self.impl.reset(new FastLEDANIMartRIX(&self));
        }
        self.impl->setTime(now);
        self.impl->loop();
    }


    static const AnimartrixEntry ANIMATION_TABLE[] = {
        {POLAR_WAVES, "POLAR_WAVES", &FastLEDANIMartRIX::Polar_Waves},
        {SPIRALUS, "SPIRALUS", &FastLEDANIMartRIX::Spiralus},
        {CALEIDO1, "CALEIDO1", &FastLEDANIMartRIX::Caleido1},
        {WAVES, "WAVES", &FastLEDANIMartRIX::Waves},
        {CHASING_SPIRALS, "CHASING_SPIRALS", &FastLEDANIMartRIX::Chasing_Spirals},
        {COMPLEX_KALEIDO_6, "COMPLEX_KALEIDO_6", &FastLEDANIMartRIX::Complex_Kaleido_6},
        {WATER, "WATER", &FastLEDANIMartRIX::Water},
        {EXPERIMENT1, "EXPERIMENT1", &FastLEDANIMartRIX::Experiment1},
        {EXPERIMENT21, "EXPERIMENT2", &FastLEDANIMartRIX::Experiment2},
        {TEST, "TEST", &FastLEDANIMartRIX::Test},
    };


    fl::string getAnimartrixName(int animation) {
        if (animation < 0 || animation >= NUM_ANIMATIONS) {
            return "UNKNOWN";
        }
        return ANIMATION_TABLE[animation].name;
    }


    void FastLEDANIMartRIX::loop() {
        for (const auto &entry : ANIMATION_TABLE) {
            if (entry.anim == data->current_animation) {
                (this->*entry.func)();
                return;
            }
        }
        // (this->*ANIMATION_TABLE[index].func)();
        FASTLED_DBG("Animation not found for " << int(data->current_animation));
    }


    const char *Animartrix::getAnimartrixName(AnimartrixAnim animation) {
        for (const auto &entry : ANIMATION_TABLE) {
            if (entry.anim == animation) {
                return entry.name;
            }
        }
        FASTLED_DBG("Animation not found for " << int(animation));
        return "UNKNOWN";
    }


    void Animartrix::draw(DrawContext ctx) {
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

} // namespace fl