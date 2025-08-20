#pragma once

#include "bleControl.h"

namespace bounce {

    bool bounceInstance = false;

    uint16_t (*xyFunc)(uint8_t x, uint8_t y);
    fl::XYMap* blurXYmap;

    void initBounce(uint16_t (*xy_func)(uint8_t, uint8_t), fl::XYMap* myXYmap) {
        bounceInstance = true;
        xyFunc = xy_func;  // Store the XY function pointer
        blurXYmap = myXYmap;
    }

    const uint8_t COUNT = 10;
    float posX[COUNT];
    float posY[COUNT];
    CRGB color;
    float velX[COUNT];
    float velY[COUNT];
    float accel[COUNT];
    byte initt = 1;

    void drawPixelXYF(float x, float y, CRGB color) {
    
        if (x < 0 || y < 0 || x > ((float) WIDTH - 1) || y > ((float) HEIGHT - 1)) return;
        
        uint8_t xx = (x - (int) x) * 255;
        uint8_t yy = (y - (int) y) * 255;
        uint8_t ix = 255 - xx;
        uint8_t iy = 255 - yy;
        
        // calculate the intensities for each affected pixel
        #define WU_WEIGHT(a, b)((uint8_t)(((a) * (b) + (a) + (b)) >> 8))
        
        uint8_t wu[4] = {
            WU_WEIGHT(ix, iy),
            WU_WEIGHT(xx, iy),
            WU_WEIGHT(ix, yy),
            WU_WEIGHT(xx, yy)
        };
    
        // multiply the intensities by the colour, and saturating-add them to the pixels
        for (uint8_t i = 0; i < 4; i++) {
            int16_t xn = x + (i & 1);
            int16_t yn = y + ((i >> 1) & 1);
            ledNum = xyFunc(xn,yn);
            CRGB clr = leds[ledNum];
            clr.r = qadd8(clr.r, (color.r * wu[i]) >> 8);
            clr.g = qadd8(clr.g, (color.g * wu[i]) >> 8);
            clr.b = qadd8(clr.b, (color.b * wu[i]) >> 8);
            leds[xyFunc(xn,yn)] = clr; 
        }
    }

    //*****************************************************************************************

    void initvalues(){
    for (int i = 0; i < COUNT; i++){
        velX[i] = (beatsin16(random(1,2) + velY[i],0,WIDTH,HEIGHT)/HEIGHT)+0.1F;
        velY[i] = (beatsin16(5 + velY[i],0,WIDTH,HEIGHT)/HEIGHT)+0.5F;
        posX[i] = random (1.01F,float(HEIGHT));
        posY[i] = random (1.0F,float(WIDTH));
        accel[i] = random(0.1F,1.0F);
    }
    }

    //*****************************************************************************************

    float newpos(){
    return random(0.1F,22.0F);
    }

    float newvel(){
    return random(0.1F,1.0F);
    }

    //*****************************************************************************************

    void runBounce() {

        if (initt == 1){
            initvalues();
            initt=0;
        }
        
            for (int i = 0; i < COUNT; i++)
            {
                if (posX[i] < 1 || posX[i] > WIDTH -2 ) 
                { 
                    velX[i] = -velX[i];
                    if (velX[i] * velX[i] > 5)
                    {
                        velX[i] = newvel();
                    }
                    if (posX[i] < 0 || posX[i] > WIDTH)
                    {
                        posX[i] = newpos();
                    }        
                }
                if (posY[i] < 1 || posY[i] > HEIGHT -2 )
                {    
                    velY[i] = -velY[i];
                    if (velY[i] * velY[i] > 10)
                    {
                        velY[i] = newvel();
                    }
                    if (posY[i] < 0 || posY[i] > HEIGHT)
                    {
                        posY[i] = newpos();
                    }        
                }
                velY[i] += 0.1F;
                posX[i] = posX[i] -  velX[i] + accel[i]; //accel[i] ;//  posX[i] = posX[i] + velX[i] * accel[i];

                posY[i] = posY[i] - velY[i] - accel[i];
                drawPixelXYF(posX[i],posY[i],ColorFromPalette(PartyColors_p,i*80,255,LINEARBLEND));    
            }

            blur2d(leds,WIDTH,HEIGHT,15, *blurXYmap);
            fadeToBlackBy(leds,NUM_LEDS,20);

    }

} // namespace bounce
