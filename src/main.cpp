//*********************************************************************************************************************************************
//*********************************************************************************************************************************************
/*
CREDITS:

Pattern functionality:
 - pride based Pride2015 by Mark Kriegsman (https://gist.github.com/kriegsman/964de772d64c502760e5)
 - waves based on ColorWavesWithPalettes by Mark Kriegsman (https://gist.github.com/kriegsman/8281905786e8b2632aeb)
 - rainboxmatrix ... trying to recall/locate where I got this; will update when I find it!
 - soapbubble based on Soap by Stepko (https://editor.soulmatelights.com/gallery/1626-soap), which was an implementation
			of an idea by StefanPetrick (https://www.youtube.com/watch?v=DiHBgITrZck&ab_channel=StefanPetrick)
 - dots based on pattern from Funky Clouds by Stefan Petrick (https://github.com/FastLED/FastLED/tree/master/examples/FunkyClouds)
 - fxWave2d based on FastLED example sketch of same name (https://github.com/FastLED/FastLED/tree/master/examples/FxWave2d) 
			by Zach Vorhies (https://github.com/zackees)
 - the "radii sketches" (octopus, flower, lotus, radialwaves) and wavingcells all based on sketches of the same names by Stepko, 
			with further credit therein to Sutaburosu (https://github.com/sutaburosu) and Stefan Petrick (https://editor.soulmatelights.com/gallery)

BLE control functionality based on esp32-fastled-ble by Jason Coons  (https://github.com/jasoncoon/esp32-fastled-ble)

In addition to each of those noted above (for the cited and other reasons), a huge thank you to Marc Miller (u/marmilicious), 
who has been of tremendous help on numerous levels!  

*/

//*********************************************************************************************************************************************
//*********************************************************************************************************************************************

#include <Arduino.h>
#include <FastLED.h>
#include "fl/sketch_macros.h"
#include "fl/xymap.h"

#include "fl/math_macros.h"  
#include "fl/time_alpha.h"  
#include "fl/ui.h"         
#include "fx/fx1d.h"
#include "fx/2d/blend.h"    
#include "fx/2d/wave.h"     

#include "palettes.h"

#include "fl/slice.h"
#include "fx/fx_engine.h"

#include <FS.h>
#include "LittleFS.h"
#define FORMAT_LITTLEFS_IF_FAILED true 

#include <Preferences.h>  
Preferences preferences;

//#define BIG_BOARD
#undef BIG_BOARD

#define DATA_PIN_1 2

//*********************************************

#ifdef BIG_BOARD 
	#include "matrixMap_32x48_3pin.h" 
	#define DATA_PIN_2 3
    #define DATA_PIN_3 4
    #define HEIGHT 32 
    #define WIDTH 48
    #define NUM_SEGMENTS 3
    #define NUM_LEDS_PER_SEGMENT 512
#else 
	#include "matrixMap_24x24.h"
	#define HEIGHT 24 
    #define WIDTH 24
    #define NUM_SEGMENTS 1
    #define NUM_LEDS_PER_SEGMENT 576
#endif


//*********************************************

#define NUM_LEDS ( WIDTH * HEIGHT )
const uint16_t MIN_DIMENSION = MIN(WIDTH, HEIGHT);
const uint16_t MAX_DIMENSION = MAX(WIDTH, HEIGHT);

CRGB leds[NUM_LEDS];
uint16_t ledNum = 0;

using namespace fl;

//bleControl variables ***********************************************************************
//elements that must be set before #include "bleControl.h" 

extern const TProgmemRGBGradientPaletteRef gGradientPalettes[]; 
extern const uint8_t gGradientPaletteCount;
uint8_t gCurrentPaletteNumber;
uint8_t gTargetPaletteNumber;
CRGBPalette16 gCurrentPalette;
CRGBPalette16 gTargetPalette;

uint8_t PROGRAM;
uint8_t MODE;	
uint8_t SPEED;
uint8_t BRIGHTNESS;
float speedfactor;

uint8_t mapping = 1;

#include "bleControl.h"

#include "animartrix.hpp"
bool animartrixFirstRun = true;

#include "bubble.hpp"
#include "radii.hpp"
#include "dots.hpp"
#include "fxWaves2d.hpp"
#include "waves.hpp"
#include "rainbow.hpp"

// Misc global variables ********************************************************************

uint8_t savedProgram;
uint8_t savedSpeed;
uint8_t savedBrightness;

// MAPPINGS **********************************************************************************

extern const uint16_t progTopDown[NUM_LEDS] PROGMEM;
extern const uint16_t progBottomUp[NUM_LEDS] PROGMEM;
extern const uint16_t serpTopDown[NUM_LEDS] PROGMEM;
extern const uint16_t serpBottomUp[NUM_LEDS] PROGMEM;

enum Mapping {
	TopDownProgressive = 1,
	TopDownSerpentine,
	BottomUpProgressive,
	BottomUpSerpentine
}; 

// General (non-FL::XYMap) mapping 
	uint16_t myXY(uint8_t x, uint8_t y) {
			if (x >= WIDTH || y >= HEIGHT) return 0;
			uint16_t i = ( y * WIDTH ) + x;
			switch(mapping){
				case 1:	 ledNum = progTopDown[i]; break;
				case 2:	 ledNum = progBottomUp[i]; break;
				case 3:	 ledNum = serpTopDown[i]; break;
				case 4:	 ledNum = serpBottomUp[i]; break;
			}
			return ledNum;
	}

// Used only for FL::XYMap purposes
	/*
	uint16_t myXYFunction(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
			width = WIDTH;
			height = HEIGHT;
			if (x >= width || y >= height) return 0;
			uint16_t i = ( y * width ) + x;

			switch(mapping){
				case 1:	 ledNum = progTopDown[i]; break;
				case 2:	 ledNum = progBottomUp[i]; break;
				case 3:	 ledNum = serpTopDown[i]; break;
				case 4:	 ledNum = serpBottomUp[i]; break;
			}
			
			return ledNum;
	}*/

	//uint16_t myXYFunction(uint16_t x, uint16_t y, uint16_t width, uint16_t height);

	//XYMap myXYmap = XYMap::constructWithUserFunction(WIDTH, HEIGHT, myXYFunction);
	XYMap myXYmap = XYMap::constructWithLookUpTable(WIDTH, HEIGHT, progBottomUp);
	XYMap xyRect = XYMap::constructRectangularGrid(WIDTH, HEIGHT);

//******************************************************************************************************************************

void setup() {
		
		preferences.begin("settings", true); // true == read only mode
			savedProgram  = preferences.getUChar("program");
			//savedMode  = preferences.getUChar("mode");
			savedBrightness  = preferences.getUChar("brightness");
			savedSpeed  = preferences.getUChar("speed");
		preferences.end();	

		//PROGRAM = 1;
		//BRIGHTNESS = 155;
		// SPEED = 5;
		PROGRAM = savedProgram;
		MODE = 0;
		BRIGHTNESS = savedBrightness;
		SPEED = savedSpeed;

		FastLED.addLeds<WS2812B, DATA_PIN_1, GRB>(leds, 0, NUM_LEDS_PER_SEGMENT)
				.setCorrection(TypicalLEDStrip);

		#ifdef DATA_PIN_2
				FastLED.addLeds<WS2812B, DATA_PIN_2, GRB>(leds, NUM_LEDS_PER_SEGMENT, NUM_LEDS_PER_SEGMENT)
				.setCorrection(TypicalLEDStrip);
		#endif
		
		#ifdef DATA_PIN_3
		FastLED.addLeds<WS2812B, DATA_PIN_3, GRB>(leds, NUM_LEDS_PER_SEGMENT * 2, NUM_LEDS_PER_SEGMENT)
				.setCorrection(TypicalLEDStrip);
		#endif

		#ifndef BIG_BOARD
			FastLED.setMaxPowerInVoltsAndMilliamps(5, 750);
		#endif
		
		FastLED.setBrightness(BRIGHTNESS);

		FastLED.clear();
		FastLED.show();

		if (debug) {
			Serial.begin(115200);
			delay(500);
			Serial.print("Initial program: ");
			Serial.println(PROGRAM);
			Serial.print("Initial brightness: ");
			Serial.println(BRIGHTNESS);
			Serial.print("Initial speed: ");
			Serial.println(SPEED);
		}

		bleSetup();

		if (!LittleFS.begin(true)) {
        	Serial.println("LittleFS mount failed!");
        	return;
		}
		Serial.println("LittleFS mounted successfully.");   

}

//*****************************************************************************************

void updateSettings_program(uint8_t newProgram){
 preferences.begin("settings",false);  // false == read write mode
	 preferences.putUChar("program", newProgram);
 preferences.end();
 savedProgram = newProgram;
 if (debug) {Serial.println("Program setting updated");}
}

//*****************************************************************************************

void updateSettings_brightness(uint8_t newBrightness){
 preferences.begin("settings",false);  // false == read write mode
	 preferences.putUChar("brightness", newBrightness);
 preferences.end();
 savedBrightness = newBrightness;
 if (debug) {Serial.println("Brightness setting updated");}
}

//*******************************************************************************************

void updateSettings_speed(uint8_t newSpeed){
 preferences.begin("settings",false);  // false == read write mode
	 preferences.putUChar("speed", newSpeed);
 preferences.end();
 savedSpeed = newSpeed;
 if (debug) {Serial.println("Speed setting updated");}
}

//**************************************************************************************************************************
// ANIMARTRIX **************************************************************************************************************
	
#define FL_ANIMARTRIX_USES_FAST_MATH 1
#define FIRST_ANIMATION CHASING_SPIRALS
Animartrix myAnimartrix(myXYmap, FIRST_ANIMATION);
FxEngine animartrixEngine(NUM_LEDS);

void setColorOrder(int value) {
    switch(value) {
        case 0: value = RGB; break;
        case 1: value = RBG; break;
        case 2: value = GRB; break;
        case 3: value = GBR; break;
        case 4: value = BRG; break;
        case 5: value = BGR; break;
    }
    myAnimartrix.setColorOrder(static_cast<EOrder>(value));
}

void runAnimartrix() { 
	FastLED.setBrightness(cBright);
	animartrixEngine.setSpeed(1);
	
	static auto lastColorOrder = -1;
	if (cColOrd != lastColorOrder) {
		setColorOrder(cColOrd);
		lastColorOrder = cColOrd;
	} 

	static auto lastFxIndex = -1;
	if (cFxIndex != lastFxIndex) {
		lastFxIndex = cFxIndex;
		myAnimartrix.fxSet(cFxIndex);
	}

	animartrixEngine.draw(millis(), leds);
}

//**************************************************************************************************************************
//**************************************************************************************************************************

void loop() {

		EVERY_N_SECONDS(30) {
			if ( BRIGHTNESS != savedBrightness ) updateSettings_brightness(BRIGHTNESS);
			if ( SPEED != savedSpeed ) updateSettings_speed(SPEED);
			if ( PROGRAM != savedProgram ) updateSettings_program(PROGRAM);
		}
 
		if (!displayOn){
			FastLED.clear();
		}
		
		else {
			
			switch(PROGRAM){

				case 0:  
					mapping = Mapping::TopDownProgressive;
					if (!rainbow::rainbowInstance) {
						rainbow::initRainbow(myXY);
					}
					rainbow::runRainbow();
					//nscale8(leds,NUM_LEDS,BRIGHTNESS);
					break; 

				case 1:
					// 1D; mapping not needed
					if (!waves::wavesInstance) {
						waves::initWaves();
					}
					waves::runWaves(); 
					break;
 
				case 2:  
					mapping = Mapping::TopDownSerpentine;
					if (!bubble::bubbleInstance) {
						bubble::initBubble(myXY);
					}
					bubble::runBubble();
					break;  

				case 3:
					mapping = Mapping::TopDownProgressive;
					if (!dots::dotsInstance) {
						dots::initDots(myXY);
					}
					dots::runDots();
					break;  
				
				case 4:
					if (!fxWaves2d::fxWaves2dInstance) {
						fxWaves2d::initFxWaves2d(myXYmap, xyRect);
					}
					fxWaves2d::runFxWaves2d();
					break;

				case 5:    
					mapping = Mapping::TopDownProgressive;
					if (!radii::radiiInstance) {
						radii::initRadii(myXY);
					}
					radii::runRadii();
					break;
				
				case 6:   
					if (animartrixFirstRun) {
						animartrixEngine.addFx(myAnimartrix);
						animartrixFirstRun = false;
					}
					runAnimartrix();
					break;

			}
		}
				
		FastLED.show();
	
		// upon BLE disconnect
		if (!deviceConnected && wasConnected) {
			if (debug) {Serial.println("Device disconnected.");}
			delay(500); // give the bluetooth stack the chance to get things ready
			pServer->startAdvertising();
			if (debug) {Serial.println("Start advertising");}
			wasConnected = false;
		}

} // loop()