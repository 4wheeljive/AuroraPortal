/*
CREDITS:
Pattern finctionality:
 - pride based Pride2015 by Mark Kriegsman (https://gist.github.com/kriegsman/964de772d64c502760e5)
 - waves based on ColorWavesWithPalettes by Mark Kriegsman (https://gist.github.com/kriegsman/8281905786e8b2632aeb)
 - rainboxmatrix ... trying to recall/locate where I got this; will update when I find it!
 - soapbubble based on Soap by Stepko (https://editor.soulmatelights.com/gallery/1626-soap), which was an implementation
   of an idea by StefanPetrick (https://www.youtube.com/watch?v=DiHBgITrZck&ab_channel=StefanPetrick)
 BLE control functionality based on esp32-fastled-ble by Jason Coons  (https://github.com/jasoncoon/esp32-fastled-ble)
*/

#include <Arduino.h>
#include <FastLED.h>
#include "fl/xymap.h"

#include "fl/math_macros.h"  
#include "fl/time_alpha.h"  
#include "fl/ui.h"         
#include "fx/2d/blend.h"    
#include "fx/2d/wave.h"     

#include "palettes.h"

#include <Preferences.h>  
Preferences preferences;

//#include <matrixMap_32x48_3pin.h>
#include <matrixMap_32x48_3pin.h>

#define HEIGHT 32   
#define WIDTH 48
#define NUM_LEDS ( WIDTH * HEIGHT )

#define NUM_SEGMENTS 3
#define NUM_LEDS_PER_SEGMENT 512

CRGB leds[NUM_LEDS];
uint16_t ledNum = 0;

using namespace fl;

// Physical configuration ************************************

#define DATA_PIN_1 2
#define DATA_PIN_2 3
#define DATA_PIN_3 4

// Dimming rings *********************************************

/*
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

const uint8_t ring0[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,23,24,35,36,47,48,59,60,71,72,83,84,85,86,87,88,89,90,91,92,93,94,95};
const uint8_t ring0size = ARRAY_SIZE(ring0);

const uint8_t ring1[] = {13,14,15,16,17,18,19,20,21,22,25,34,37,46,49,58,61,70,73,74,75,76,77,78,79,80,81,82};
const uint8_t ring1size = ARRAY_SIZE(ring1);

const uint8_t ring2[] = {26,27,28,31,32,33,38,45,50,57,62,63,64,67,68,69};
const uint8_t ring2size = ARRAY_SIZE(ring2);

const uint8_t ring3[] = {29,30,39,40,41,42,43,44,51,52,53,54,55,56,65,66};
const uint8_t ring3size = ARRAY_SIZE(ring3);
*/

//bleControl variables ****************************************************************************
//elements that must be set before #include "bleControl.h" 

uint8_t program = 2;
uint8_t pattern;
bool displayOn = true;
bool runPride = true;
bool runWaves = false;
bool rotateWaves = true; 
bool triggerFancy = false;

extern const TProgmemRGBGradientPaletteRef gGradientPalettes[]; 
extern const uint8_t gGradientPaletteCount;

uint8_t gCurrentPaletteNumber;
CRGBPalette16 gCurrentPalette;
CRGBPalette16 gTargetPalette;

uint8_t SPEED;
float speedfactor;
uint8_t BRIGHTNESS;
const uint8_t brightnessInc = 15;
bool brightnessChanged = false;

#include "bleControl.h"

// Misc global variables ********************************************************************

uint8_t blendFract = 64;
uint16_t hueIncMax = 1500;
CRGB newcolor = CRGB::Black;

uint8_t savedSpeed;
uint8_t savedBrightness;

#define SECONDS_PER_PALETTE 20

// MAPPINGS **********************************************************************************

extern const uint16_t loc2indSerpByRow[32][48] PROGMEM;
extern const uint16_t loc2indProgByRow[32][48] PROGMEM;
extern const uint16_t loc2indSerp[1536] PROGMEM;
extern const uint16_t loc2indProg[1536] PROGMEM;
extern const uint16_t loc2indProgByColBottomUp[48][32] PROGMEM;

uint16_t XY(uint8_t x, uint8_t y) {
    ledNum = loc2indProgByColBottomUp[x][y];
    return ledNum;
}


uint16_t dotsXY(uint16_t x, uint16_t y) { 
  if (x >= WIDTH || y >= HEIGHT) return 0;
  uint8_t yFlip = (HEIGHT - 1) - y ;             // comment/uncomment to flip direction of vertical motion
  return loc2indProgByColBottomUp[x][yFlip];
}

// For XYMap custom mapping
uint16_t myXYFunction(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
    width = WIDTH;
    height = HEIGHT;
    if (x >= width || y >= height) return 0;
    ledNum = loc2indProgByColBottomUp[x][y];
    return ledNum;
}


//extern uint16_t loc2indProgByColBottomUp[48][32];

uint16_t myXYFunction(uint16_t x, uint16_t y, uint16_t width, uint16_t height);

XYMap myXYmap = XYMap::constructWithUserFunction(WIDTH, HEIGHT, myXYFunction);
XYMap xyRect = XYMap::constructRectangularGrid(WIDTH, HEIGHT);


//********************************************************************************************

void setup() {
    
    preferences.begin("settings", true); // true == read only mode
      savedBrightness  = preferences.getUChar("brightness");
      savedSpeed  = preferences.getUChar("speed");
    preferences.end();

    //BRIGHTNESS = 155;
    // SPEED = 5;
    BRIGHTNESS = savedBrightness;
    SPEED = savedSpeed;

    FastLED.addLeds<WS2812B, DATA_PIN_1, GRB>(leds, 0, NUM_LEDS_PER_SEGMENT)
        .setCorrection(TypicalLEDStrip);

    FastLED.addLeds<WS2812B, DATA_PIN_2, GRB>(leds, NUM_LEDS_PER_SEGMENT, NUM_LEDS_PER_SEGMENT)
        .setCorrection(TypicalLEDStrip);
        
    FastLED.addLeds<WS2812B, DATA_PIN_3, GRB>(leds, NUM_LEDS_PER_SEGMENT * 2, NUM_LEDS_PER_SEGMENT)
        .setCorrection(TypicalLEDStrip);

    FastLED.setBrightness(BRIGHTNESS);

    FastLED.clear();
    FastLED.show();

    Serial.begin(115200);
    delay(500);
    Serial.print("Initial brightness: ");
    Serial.println(BRIGHTNESS);
    Serial.print("Initial speed: ");
    Serial.println(SPEED);

    bleSetup();

}

//*****************************************************************************************

void updateSettings_brightness(uint8_t newBrightness){
 preferences.begin("settings",false);  // false == read write mode
   preferences.putUChar("brightness", newBrightness);
 preferences.end();
 savedBrightness = newBrightness;
 Serial.println("Brightness setting updated");
}

//*******************************************************************************************

void updateSettings_speed(uint8_t newSpeed){
 preferences.begin("settings",false);  // false == read write mode
   preferences.putUChar("speed", newSpeed);
 preferences.end();
 savedSpeed = newSpeed;
 Serial.println("Speed setting updated");
}

// *************************************************************************************************************************
// PRIDE/WAVES**************************************************************************************************************
// Code matrix format: 1D, Serpentine

void prideWaves(uint8_t pattern) {

  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;
 
  uint8_t sat8 = beatsin88( 87, 240, 255); 
  uint8_t brightdepth = beatsin88( 341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60); 
 
  uint16_t hue16 = sHue16; 
  uint16_t hueinc16 = beatsin88(113, 1, hueIncMax);    
  uint16_t ms = millis();  
  uint16_t deltams = ms - sLastMillis ;
  sLastMillis  = ms;     
  sPseudotime += deltams * msmultiplier*speedfactor;
  sHue16 += deltams * beatsin88( 400, 5,9);
  uint16_t brightnesstheta16 = sPseudotime;

  for( uint16_t i = 0 ; i < NUM_LEDS; i++ ) {
   
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;

    if (runWaves) {
      uint16_t h16_128 = hue16 >> 7;
      if( h16_128 & 0x100) {
        hue8 = 255 - (h16_128 >> 1);
      } else {
        hue8 = h16_128 >> 1;
      }
    }

    brightnesstheta16  += brightnessthetainc16;
    uint16_t b16 = sin16( brightnesstheta16  ) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);
  
    switch (pattern) {
  
      case 1:
        newcolor = CHSV( hue8, sat8, bri8);
        blendFract = 64;
        break;
  
      case 2:
        uint8_t index = hue8;
        index = scale8( index, 240);
        newcolor = ColorFromPalette( gCurrentPalette, index, bri8);
        blendFract = 128;
        break;
   }
        
   uint16_t pixelnumber = i;
   pixelnumber = (NUM_LEDS-1) - pixelnumber;  // comment/uncomment this line to reverse apparent direction of LED progression   
   uint16_t ledNum = loc2indSerp[pixelnumber];
   nblend( leds[ledNum], newcolor, blendFract);
   
  }

}
// *************************************************************************************************************************
// RAINBOW MATRIX **********************************************************************************************************
// Code matrix format: 2D, Needs SerpByRow for 8x12

void DrawOneFrame( uint8_t startHue8, int8_t yHueDelta8, int8_t xHueDelta8) {
  uint8_t lineStartHue = startHue8;
  for( uint8_t y = 0; y < HEIGHT; y++) {
    lineStartHue += yHueDelta8;
    uint8_t pixelHue = lineStartHue;      
    for( uint8_t x = 0; x < WIDTH; x++) {
      pixelHue += xHueDelta8;
      leds[loc2indSerpByRow[y][x]] = CHSV(pixelHue, 255, 255);
    }  
  }
}

void rainbowMatrix () {
    uint32_t ms = millis();
    int32_t yHueDelta32 = ((int32_t)cos16( ms * (27/1) ) * (350 / WIDTH));
    int32_t xHueDelta32 = ((int32_t)cos16( ms * (39/1) ) * (310 / HEIGHT));
    DrawOneFrame( ms / 65536, yHueDelta32 / 32768, xHueDelta32 / 32768);
 }

// *************************************************************************************************************************
// SOAP BUBBLE**************************************************************************************************************

int8_t zD;
int8_t zF;
uint8_t noise3d[WIDTH][HEIGHT];
uint32_t noise32_x;
uint32_t noise32_y;

uint32_t noise32_z;

uint32_t scale32_x;
uint32_t scale32_y;
bool isSetup = 1;
uint8_t noisesmooth;

void FillNoise() {
  for (uint8_t i = 0; i < WIDTH; i++) {
    int32_t ioffset = scale32_x * (i - WIDTH / 2);
    for (uint8_t j = 0; j < HEIGHT; j++) {
      int32_t joffset = scale32_y * (j - HEIGHT / 2);
      int8_t data = inoise16(noise32_x + ioffset, noise32_y + joffset, noise32_z) >> 8;
      int8_t olddata = noise3d[i][j];
      int8_t newdata = scale8(olddata, noisesmooth) + scale8(data, 255 - noisesmooth);
      data = newdata;
      noise3d[i][j] = data;     // puts a uint8_t hue value into position [i][j] in the noise3d array
    }
  }
}

void MoveFractionalNoiseX(int8_t amplitude = 1, float shift = 0) {
  CRGB ledsbuff[WIDTH];
  for (uint8_t y = 0; y < HEIGHT; y++) {
    int16_t amount = ((int16_t) noise3d[0][y] - 128) * 2 * amplitude + shift * 256;
    int8_t delta = abs(amount) >> 8;
    int8_t fraction = abs(amount) & 255;
    for (uint8_t x = 0; x < WIDTH; x++) {
      if (amount < 0) {
        zD = x - delta;
        zF = zD - 1;
      } else {
        zD = x + delta;
        zF = zD + 1;
      }
      CRGB PixelA = CRGB::Black;
      if ((zD >= 0) && (zD < WIDTH)) {
        PixelA = leds[XY(zD, y)];
      } 
      else {
        PixelA = CHSV(~noise3d[abs(zD)%WIDTH][y]*3,255,255);
      }
      CRGB PixelB = CRGB::Black;
      if ((zF >= 0) && (zF < WIDTH)) {
        PixelB = leds[XY(zF, y)]; 
      }
      else {
        PixelB = CHSV(~noise3d[abs(zF)%WIDTH][y]*3,255,255);
      }
      ledsbuff[x] = (PixelA.nscale8(ease8InOutApprox(255 - fraction))) + (PixelB.nscale8(ease8InOutApprox(fraction))); // lerp8by8(PixelA, PixelB, fraction );
    }
    for (uint8_t x = 0; x < WIDTH; x++) {
      leds[XY(x, y)] = ledsbuff[x];
    }
  }
}

void MoveFractionalNoiseY(int8_t amplitude = 1, float shift = 0) {
  CRGB ledsbuff[HEIGHT];
  for (uint8_t x = 0; x < WIDTH; x++) {
    int16_t amount = ((int16_t) noise3d[x][0] - 128) * 2 * amplitude + shift * 256;
    int8_t delta = abs(amount) >> 8;
    int8_t fraction = abs(amount) & 255;
    for (uint8_t y = 0; y < HEIGHT; y++) {
      if (amount < 0) {
        zD = y - delta;
        zF = zD - 1;
      } else {
        zD = y + delta;
        zF = zD + 1;
      }
      CRGB PixelA = CRGB::Black;
      if ((zD >= 0) && (zD < HEIGHT)) PixelA = leds[XY(x, zD)]; else PixelA = CHSV(~noise3d[x][abs(zD)%HEIGHT]*3,255,255); 
      CRGB PixelB = CRGB::Black;
      if ((zF >= 0) && (zF < HEIGHT))PixelB = leds[XY(x, zF)];  else PixelB = CHSV(~noise3d[x][abs(zF)%HEIGHT]*3,255,255);
      ledsbuff[y] = (PixelA.nscale8(ease8InOutApprox(255 - fraction))) + (PixelB.nscale8(ease8InOutApprox(fraction)));
    }
    for (uint8_t y = 0; y < HEIGHT; y++) {
      leds[XY(x, y)] = ledsbuff[y];
    }
  }
}

uint16_t mov = max(WIDTH,HEIGHT)*47;

void soapBubble() {
  if (isSetup) {
  noise32_x = random16();
  noise32_y = random16();
  noise32_z = random16();
    scale32_x = 160000/WIDTH;
    scale32_y = 160000/HEIGHT;
    FillNoise();
    for (byte i = 0; i < WIDTH; i++) {
      for (byte j = 0; j < HEIGHT; j++) {
        leds[XY(i, j)] = CHSV(~noise3d[i][j]*3,255,255);
        //leds[XY(i, j)] = CHSV(newhue,255,255);
      }
    }
    isSetup = 0;
  }
  noise32_x += mov;
  noise32_y += mov;
  noise32_z += mov;
  FillNoise();
  FastLED.delay(5);
  MoveFractionalNoiseX(WIDTH/8);
  MoveFractionalNoiseY(HEIGHT/8);
}


// *************************************************************************************************************************** 
// DOT DANCE *****************************************************************************************************************

// the oscillators: linear ramps 0-255
// modified only by MoveOscillators()
byte osci[4]; 

// sin8(osci) swinging between 0 - ???
// modified only by MoveOscillators()
byte p[4];

//****************************************************************************************************

void PixelA(uint16_t x, uint16_t y, byte color) {
  leds[dotsXY(x, y)] = CHSV(color, 255, 255);
}

void PixelB(uint16_t x, uint16_t y, byte color) {
  leds[dotsXY(x, y)] = CHSV(color, 255, 255);
}

// set the speeds (and by that ratios) of the oscillators here
void MoveOscillators() {
  osci[0] = osci[0] + 5;
  osci[1] = osci[1] + 2;
  osci[2] = osci[2] + 3;
  osci[3] = osci[3] + 4;
  for(int i = 0; i < 4; i++) { 
    p[i] = sin8(osci[i])/8;//  keep the result in the range of 0-??? (matrix size)
  }
}

// give it a linear tail downwards
void VerticalStream(byte scale)  
{
  for(uint16_t x = 0; x < WIDTH ; x++) {
    for(uint16_t y = 1; y < HEIGHT; y++) {
      leds[dotsXY(x,y)] += leds[dotsXY(x,y-1)];
      leds[dotsXY(x,y)].nscale8( scale );
    }
  }
  for(uint16_t x = 0; x < WIDTH; x++) 
    leds[dotsXY(x,0)].nscale8(scale);
}

//*****************************************************************************

void dotDance() {

  MoveOscillators();

  PixelA( 
    (p[2]+p[0]+p[1])/2,    //  /2 for JSH duct tape adjust for screen width > MAX DIMENSION 
    (p[1]+p[3]+p[2])/3,
    osci[1]
  );

  PixelB( 
    (p[3]+p[0]+p[1])/2,    // /2 for JSH duct tape adjust for screen width > MAX DIMENSION 
    (p[1]+p[0]+p[2])/3,
    osci[3]
  );
   
  VerticalStream(125);
  FastLED.delay(10);
 
}

// *************************************************************************************************************************** 
// fxWave2d *****************************************************************************************************************

bool firstWave = true;
UICheckbox xCyclical("X Is Cyclical", false);
UIButton button("Trigger");                              

UICheckbox autoTrigger("Auto Trigger", true);   
UISlider triggerSpeed("Trigger Speed", .4f, 0.0f, 1.0f, 0.01f);
UICheckbox easeModeSqrt("Ease Mode Sqrt", false);    
UISlider blurAmount("Global Blur Amount", 0, 0, 172, 1);  
UISlider blurPasses("Global Blur Passes", 1, 1, 10, 1);   
UISlider superSample("SuperSampleExponent", 1.f, 0.f, 3.f, 1.f); 
  
// Lower wave layer controls:
UISlider speedLower("Wave Lower: Speed", 0.26f, 0.0f, 1.0f);      
UISlider dampeningLower("Wave Lower: Dampening", 8.0f, 0.0f, 20.0f, 0.1f); 
UICheckbox halfDuplexLower("Wave Lower: Half Duplex", true);         
UISlider blurAmountLower("Wave Lower: Blur Amount", 0, 0, 172, 1);  
UISlider blurPassesLower("Wave Lower: Blur Passes", 1, 1, 10, 1);    

// Upper wave layer controls:
UISlider speedUpper("Wave Upper: Speed", 0.15f, 0.0f, 1.0f);     
UISlider dampeningUpper("Wave Upper: Dampening", 7.9f, 0.0f, 20.0f, 0.1f); 
UICheckbox halfDuplexUpper("Wave Upper: Half Duplex", true);     
UISlider blurAmountUpper("Wave Upper: Blur Amount", 50, 0, 172, 1); 
UISlider blurPassesUpper("Wave Upper: Blur Passes", 1, 1, 10, 1);   

// Fancy effect controls (for the cross-shaped effect):
UISlider fancySpeed("Fancy Speed", 796, 0, 1000, 1);  
UISlider fancyIntensity("Fancy Intensity", 32, 1, 255, 1);
UISlider fancyParticleSpan("Fancy Particle Span", 0.06f, 0.01f, 0.2f, 0.01f);

UIButton buttonFancy("Trigger Fancy");

//************************************************************************************************************

DEFINE_GRADIENT_PALETTE(electricBlueFirePal){
    0,   0,   0,   0,   
    32,  0,   0,   70,  
    128, 20,  57,  255, 
    255, 255, 255, 255 
};

DEFINE_GRADIENT_PALETTE(electricGreenFirePal){
    0,   0,   0,   0,  
    8,   128, 64,  64,  
    16,  225, 200, 200, 
    100,  250, 250, 250, 
    255, 255, 255, 255  
};

//************************************************************************************************************

WaveFx::Args CreateArgsLower() {
    WaveFx::Args out;
    out.factor = SuperSample::SUPER_SAMPLE_8X;
    out.half_duplex = true;
    out.auto_updates = true;  
    out.speed = 0.16f; 
    out.dampening = 9.0f;
    out.crgbMap = WaveCrgbGradientMapPtr::New(electricBlueFirePal);  
    return out;
}  

WaveFx::Args CreateArgsUpper() {
    WaveFx::Args out;
    out.factor = SuperSample::SUPER_SAMPLE_8X; 
    out.half_duplex = true;  
    out.auto_updates = true; 
    out.speed = 0.26f;      
    out.dampening = 5.0f;     
    out.crgbMap = WaveCrgbGradientMapPtr::New(electricGreenFirePal); 
    return out;
}

// For screenTest, these need to use xyRect
// For LED panel display, these need to use myXYmap
WaveFx waveFxLower(myXYmap, CreateArgsLower());
WaveFx waveFxUpper(myXYmap, CreateArgsUpper()); 

// For screenTest, this needs to use myXYmap
// For LED panel display, this needs to use xyRect
Blend2d fxBlend(xyRect);

//************************************************************************************************************

SuperSample getSuperSample() {
    switch (int(superSample)) {
    case 0:
        return SuperSample::SUPER_SAMPLE_NONE;
    case 1:
        return SuperSample::SUPER_SAMPLE_2X;
    case 2:
        return SuperSample::SUPER_SAMPLE_4X;
    case 3:
        return SuperSample::SUPER_SAMPLE_8X;
    default:
        return SuperSample::SUPER_SAMPLE_NONE;
    }
}

//************************************************************************************************************

void triggerRipple() {

    float perc = .15f;
  
    uint8_t min_x = perc * WIDTH;    
    uint8_t max_x = (1 - perc) * WIDTH; 
    uint8_t min_y = perc * HEIGHT;     
    uint8_t max_y = (1 - perc) * HEIGHT; 
   
    int x = random(min_x, max_x);
    int y = random(min_y, max_y);
    
    waveFxLower.setf(x, y, 1); 
    waveFxUpper.setf(x, y, 1);

}

//************************************************************************************************************


void applyFancyEffect(uint32_t now, bool button_active) {
   
    uint32_t total =
        map(fancySpeed.as<uint32_t>(), 0, fancySpeed.getMax(), 1000, 100);
    
    static TimeRamp pointTransition = TimeRamp(total, 0, 0);

    if (button_active) {
        pointTransition.trigger(now, total, 0, 0);
    }

    // If the animation isn't currently active, exit early
    if (!pointTransition.isActive(now)) {
        // no need to draw
        return;
    }

    int mid_x = WIDTH / 2;
    int mid_y = HEIGHT / 2;
    
    int amount = WIDTH / 2;

    int start_x = mid_x - amount;  
    int end_x = mid_x + amount;  
    int start_y = mid_y - amount; 
    int end_y = mid_y + amount; 
    
    int curr_alpha = pointTransition.update8(now);
    
    int left_x = map(curr_alpha, 0, 255, mid_x, start_x);  
    int down_y = map(curr_alpha, 0, 255, mid_y, start_y);  
    int right_x = map(curr_alpha, 0, 255, mid_x, end_x);  
    int up_y = map(curr_alpha, 0, 255, mid_y, end_y);    

    float curr_alpha_f = curr_alpha / 255.0f;

    float valuef = (1.0f - curr_alpha_f) * fancyIntensity.value() / 255.0f;

    int span = fancyParticleSpan.value() * WIDTH;
 
    for (int x = left_x - span; x < left_x + span; x++) {
        waveFxLower.addf(x, mid_y, valuef); 
        waveFxUpper.addf(x, mid_y, valuef); 
    }

    for (int x = right_x - span; x < right_x + span; x++) {
        waveFxLower.addf(x, mid_y, valuef);
        waveFxUpper.addf(x, mid_y, valuef);
    }

    for (int y = down_y - span; y < down_y + span; y++) {
        waveFxLower.addf(mid_x, y, valuef);
        waveFxUpper.addf(mid_x, y, valuef);
    }

    for (int y = up_y - span; y < up_y + span; y++) {
        waveFxLower.addf(mid_x, y, valuef);
        waveFxUpper.addf(mid_x, y, valuef);
    }
}

struct ui_state {
    bool button = false;
    bool bigButton = false;
};

ui_state ui() {
    
    U8EasingFunction easeMode = easeModeSqrt
                                    ? U8EasingFunction::WAVE_U8_MODE_SQRT
                                    : U8EasingFunction::WAVE_U8_MODE_LINEAR;
    
    waveFxLower.setSpeed(speedLower);             
    waveFxLower.setDampening(dampeningLower);      
    waveFxLower.setHalfDuplex(halfDuplexLower);    
    waveFxLower.setSuperSample(getSuperSample());  
    waveFxLower.setEasingMode(easeMode);           
    
    waveFxUpper.setSpeed(speedUpper);             
    waveFxUpper.setDampening(dampeningUpper);     
    waveFxUpper.setHalfDuplex(halfDuplexUpper);   
    waveFxUpper.setSuperSample(getSuperSample()); 
    waveFxUpper.setEasingMode(easeMode);      
   
    fxBlend.setGlobalBlurAmount(blurAmount);      
    fxBlend.setGlobalBlurPasses(blurPasses);     

    Blend2dParams lower_params = {
        .blur_amount = blurAmountLower,            
        .blur_passes = blurPassesLower,         
    };

    Blend2dParams upper_params = {
        .blur_amount = blurAmountUpper,        
        .blur_passes = blurPassesUpper,           
    };

    fxBlend.setParams(waveFxLower, lower_params);
    fxBlend.setParams(waveFxUpper, upper_params);

     ui_state state{
        .button = button,
        .bigButton = triggerFancy,   
    };
    return state;
}


void processAutoTrigger(uint32_t now) {
  
    static uint32_t nextTrigger = 0;
    
    uint32_t trigger_delta = nextTrigger - now;
    
    if (trigger_delta > 10000) {
        trigger_delta = 0;
    }
    
    if (autoTrigger) {
        if (now >= nextTrigger) {
            triggerRipple();
  
            float speed = 1.0f - triggerSpeed.value();
            uint32_t min_rand = 300 * speed; 
            uint32_t max_rand = 2000 * speed; 

            uint32_t min = MIN(min_rand, max_rand);
            uint32_t max = MAX(min_rand, max_rand);
            
            if (min == max) {
                max += 1;
            }
            
            nextTrigger = now + random(min, max);
        }
    }
}

//********************************************************************************************

void loop() {

    EVERY_N_SECONDS(30) {
      if ( BRIGHTNESS != savedBrightness ) updateSettings_brightness(BRIGHTNESS);
      if ( SPEED != savedSpeed ) updateSettings_speed(SPEED);
    }
 
    if (!displayOn){
      FastLED.clear();
      FastLED.show();
    }
    else {
      
      switch(program){

        case 1:  
          rainbowMatrix ();
          nscale8(leds,NUM_LEDS,BRIGHTNESS);
          break; 

        case 2:
          if (runPride) { 
            hueIncMax = 500;
            prideWaves(1); 
          }
    
          if (runWaves) { 
            hueIncMax = 1500;
            if (rotateWaves) {
              EVERY_N_SECONDS( SECONDS_PER_PALETTE ) {
                gCurrentPaletteNumber = addmod8( gCurrentPaletteNumber, 1, gGradientPaletteCount);
                gTargetPalette = gGradientPalettes[ gCurrentPaletteNumber ];
                pPaletteCharacteristic->setValue(String(gCurrentPaletteNumber).c_str());
                pPaletteCharacteristic->notify();
                Serial.print("Color palette: ");
                Serial.println(gCurrentPaletteNumber);
              }
            }
            EVERY_N_MILLISECONDS(40) {
                nblendPaletteTowardPalette( gCurrentPalette, gTargetPalette, 16); 
            }
            prideWaves(2); 
          }
          break;
 
        case 3:  
          soapBubble();
          break;  

        case 4:  
          dotDance();
          break;  
        
        case 5:
          if (firstWave) {
            fxBlend.add(waveFxLower);
            fxBlend.add(waveFxUpper);
            firstWave = false;
          }
          uint32_t now = millis();
          waveFxLower.setXCylindrical(xCyclical.value());  
          ui_state state = ui();
          applyFancyEffect(now,state.bigButton);
          processAutoTrigger(now);
          Fx::DrawContext ctx(now, leds);
          fxBlend.draw(ctx);  
          break;
  
      }

      FastLED.show();
    }

    // while connected
    if (deviceConnected) {
      if (brightnessChanged) { 
        pBrightnessCharacteristic->notify();
        brightnessChanged = false;
      }
    }

    // upon disconnect
    if (!deviceConnected && wasConnected) {
      Serial.println("Device disconnected.");
      delay(500); // give the bluetooth stack the chance to get things ready
      pServer->startAdvertising();
      Serial.println("Start advertising");
      wasConnected = false;
    }

}