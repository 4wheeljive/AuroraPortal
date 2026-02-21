//*********************************************************************************************************************************************
/*
CREDITS:

Pattern functionality:
 - pride based Pride2015 by Mark Kriegsman (https://gist.github.com/kriegsman/964de772d64c502760e5)
 - waves based on ColorWavesWithPalettes by Mark Kriegsman (https://gist.github.com/kriegsman/8281905786e8b2632aeb)
 - rainboxmatrix based on FastLED XYMatrix example (https://github.com/FastLED/FastLED/blob/master/examples/XYMatrix/XYMatrix.ino)
 - soapbubble based on Soap by Stepko (https://editor.soulmatelights.com/gallery/1626-soap), which was an implementation
			of an idea by Stefan Petrick (https://www.youtube.com/watch?v=DiHBgITrZck&ab_channel=StefanPetrick)
 - dots based on pattern from Funky Clouds by Stefan Petrick (https://github.com/FastLED/FastLED/tree/master/examples/FunkyClouds)
 - fxWave2d based on FastLED example sketch of same name (https://github.com/FastLED/FastLED/tree/master/examples/FxWave2d) 
			by Zach Vorhies (https://github.com/zackees)
 - the "radii sketches" (octopus, flower, lotus, radialwaves) and wavingcells all based on sketches of the same names by Stepko, 
			with further credit therein to Sutaburosu (https://github.com/sutaburosu) and Stefan Petrick (https://editor.soulmatelights.com/gallery)
 - animARTrix engine and patterns based on the FastLED implementation of Stefan Petrick's creation of the same name
 			Further credits in animartrix_detail.hpp   
 - synaptide inbspired by WaveScene by Knifa Dan (https://github.com/Knifa/matryx-gl)
 - cube based on AI-generated code shared by Fluffy-Wishbone-3497 here: 
 			https://www.reddit.com/r/FastLED/comments/1nvuzjg/claude_does_like_to_code_fastled/

BLE control functionality based on esp32-fastled-ble by Jason Coons  (https://github.com/jasoncoon/esp32-fastled-ble)

In addition to each of those noted above (for the cited and other reasons), a huge thank you to Marc Miller (u/marmilicious), 
who has been of tremendous help on numerous levels!  

*/

//*********************************************************************************************************************************************

#include <Arduino.h>

//#define FASTLED_OVERCLOCK 1.2
#include <FastLED.h>

#include "fl/sketch_macros.h"
#include "fl/xymap.h"

#include "fl/math_macros.h"  
#include "fl/time_alpha.h"  
#include "fl/ui.h"         
#include "fl/fx/fx.h" 		
#include "fl/fx/2d/blend.h"	
#include "fl/fx/2d/wave.h"	
#include "fl/fx/fx2d.h"  

#include "reference/palettes.h"

#include "fl/slice.h"
//#include "fl/fx/fx_engine.h" 

#include <FS.h>
#include "LittleFS.h"
#define FORMAT_LITTLEFS_IF_FAILED true 

#include <Preferences.h>
Preferences preferences;

bool debug = true;

//#include "profiler.h"
//SimpleProfiler profiler;

//#define BIG_BOARD
#undef BIG_BOARD

#define PIN0 2

//*********************************************

#ifdef BIG_BOARD 
	
	#include "reference/matrixMap_32x48_3pin.h" 
	#define PIN1 3
    #define PIN2 4
    #define HEIGHT 32 
    #define WIDTH 48
    #define NUM_STRIPS 3
    #define NUM_LEDS_PER_STRIP 512
	#define BUS_ROWS 10
			
#else 
	
	#include "reference/matrixMap_22x22.h"
	#define HEIGHT 22 
    #define WIDTH 22
    #define NUM_STRIPS 1
    #define NUM_LEDS_PER_STRIP 484
	#define BUS_ROWS 7

#endif

//*********************************************

#define NUM_LEDS ( WIDTH * HEIGHT )
const uint16_t MIN_DIMENSION = min(WIDTH, HEIGHT);
const uint16_t MAX_DIMENSION = max(WIDTH, HEIGHT);

fl::CRGB leds[NUM_LEDS];
uint16_t ledNum = 0;

//bleControl variables ***********************************************************************
//elements that must be set before #include "bleControl.h" 

extern const TProgmemRGBGradientPaletteRef gGradientPalettes[]; 
extern const uint8_t gGradientPaletteCount;
uint8_t gCurrentPaletteNumber;
uint8_t gTargetPaletteNumber;
fl::CRGBPalette16 gCurrentPalette;
fl::CRGBPalette16 gTargetPalette;

uint8_t PROGRAM;
uint8_t MODE;
uint8_t SPEED;
uint8_t BRIGHTNESS;

uint8_t defaultMapping = 0;
bool mappingOverride = false;

#include "audio/audioInput.h"
#include "audio/audioProcessing.h"
#include "bleControl.h"

#include "programs/rainbow.hpp"
#include "programs/waves.hpp"
#include "programs/bubble.hpp"
#include "programs/dots.hpp"
#include "programs/radii.hpp"
#include "programs/fxWave2d.hpp"
#include "programs/animartrix.hpp"
#include "programs/test.hpp"
#include "programs/synaptide.hpp"
#include "programs/cube.hpp"
#include "programs/horizons.hpp"
#include "programs/audioTest.hpp"

using namespace fl;

//*****************************************************************************************
// Misc global variables ********************************************************************

//uint8_t savedSpeed;
uint8_t savedBrightness;
uint8_t savedProgram;
uint8_t savedMode;

// MAPPINGS **********************************************************************************

extern const uint16_t progTopDown[NUM_LEDS] PROGMEM;
extern const uint16_t progBottomUp[NUM_LEDS] PROGMEM;
extern const uint16_t serpTopDown[NUM_LEDS] PROGMEM;
extern const uint16_t serpBottomUp[NUM_LEDS] PROGMEM;

enum Mapping {
	TopDownProgressive = 0,
	TopDownSerpentine,
	BottomUpProgressive,
	BottomUpSerpentine
};

// General (non-FL::XYMap) mapping 
	
	uint16_t myXY(uint8_t x, uint8_t y) {
			if (x >= WIDTH || y >= HEIGHT) return 0;
			uint16_t i = ( y * WIDTH ) + x;
			switch(cMapping){
				case 0:	 ledNum = progTopDown[i]; break;
				case 1:	 ledNum = progBottomUp[i]; break;
				case 2:	 ledNum = serpTopDown[i]; break;
				case 3:	 ledNum = serpBottomUp[i]; break;
				//case 4:	 ledNum = vProgTopDown[i]; break;
				//case 5:	 ledNum = vSerpTopDown[i]; break;
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


void setup() {
		
	Serial.begin(115200);
	delay(1000);
	
	preferences.begin("settings", true); // true == read only mode
		savedBrightness  = preferences.getUChar("brightness");
		//savedSpeed  = preferences.getUChar("speed");
		savedProgram  = preferences.getUChar("program");
		savedMode  = preferences.getUChar("mode");
	preferences.end();

	PROGRAM = 6;
	MODE = 5;
	BRIGHTNESS = 35;
	//PROGRAM = savedProgram;
	//MODE = savedMode;
	
	FastLED.setExclusiveDriver("RMT");

	FastLED.addLeds<WS2812B, PIN0, GRB>(leds, 0, NUM_LEDS_PER_STRIP)
		.setCorrection(TypicalLEDStrip);

	#ifdef PIN1
		FastLED.addLeds<WS2812B, PIN1, GRB>(leds, NUM_LEDS_PER_STRIP, NUM_LEDS_PER_STRIP)
			.setCorrection(TypicalLEDStrip);
	#endif
	
	#ifdef PIN2
		FastLED.addLeds<WS2812B, PIN2, GRB>(leds, NUM_LEDS_PER_STRIP * 2, NUM_LEDS_PER_STRIP)
			.setCorrection(TypicalLEDStrip);
	#endif

	#ifdef PIN3
		FastLED.addLeds<WS2812B, PIN3, GRB>(leds, NUM_LEDS_PER_STRIP * 3, NUM_LEDS_PER_STRIP)
			.setCorrection(TypicalLEDStrip);
	#endif

	#ifdef PIN4
		FastLED.addLeds<WS2812B, PIN4, GRB>(leds, NUM_LEDS_PER_STRIP * 4, NUM_LEDS_PER_STRIP)
			.setCorrection(TypicalLEDStrip);
	#endif

	#ifdef PIN5
		FastLED.addLeds<WS2812B, PIN5, GRB>(leds, NUM_LEDS_PER_STRIP * 5, NUM_LEDS_PER_STRIP)
			.setCorrection(TypicalLEDStrip);
	#endif
	
	#ifndef BIG_BOARD
		FastLED.setMaxPowerInVoltsAndMilliamps(5, 750);
	#endif
	
	FastLED.setBrightness(BRIGHTNESS);

	FastLED.clear();
	FastLED.show();

	bleSetup();

	if (!LittleFS.begin(true)) {
		Serial.println("LittleFS mount failed!");
		return;
	}
	Serial.println("LittleFS mounted successfully.");

	myAudio::initAudioInput();
	myAudio::initAudioProcessing();

}

//*****************************************************************************************

void updateSettings_brightness(uint8_t newBrightness){
	preferences.begin("settings",false);  // false == read write mode
		preferences.putUChar("brightness", newBrightness);
	preferences.end();
	savedBrightness = newBrightness;
	//if (debug) {Serial.println("Brightness setting updated");}
	FASTLED_DBG("Brightness setting updated");
}

//*******************************************************************************************

/*void updateSettings_speed(uint8_t newSpeed){
 preferences.begin("settings",false);  // false == read write mode
	 preferences.putUChar("speed", newSpeed);
 preferences.end();
 savedSpeed = newSpeed;
 if (debug) {Serial.println("Speed setting updated");}
}*/

//*****************************************************************************************

void updateSettings_program(uint8_t newProgram){
	preferences.begin("settings",false);  // false == read write mode
		preferences.putUChar("program", newProgram);
	preferences.end();
	savedProgram = newProgram;
 FASTLED_DBG("Program setting updated");
}

//*****************************************************************************************

void updateSettings_mode(uint8_t newMode){
	preferences.begin("settings",false);  // false == read write mode
		preferences.putUChar("mode", newMode);
	preferences.end();
	savedMode = newMode;
	//if (debug) {Serial.println("Mode setting updated");}
	FASTLED_DBG("Mode setting updated");
}

//*****************************************************************************************

void loop() {

	/*
	EVERY_N_SECONDS(3) {
		uint8_t fps = FastLED.getFPS();
		FASTLED_DBG(fps << " fps");
	}
	
	EVERY_N_SECONDS(10) {
	 	FASTLED_DBG("Program: " << PROGRAM);
		FASTLED_DBG("Mode: " << MODE);
		//profiler.printStats();
		//profiler.reset();
	}*/

	EVERY_N_SECONDS(30) {
		if ( BRIGHTNESS != savedBrightness ) updateSettings_brightness(BRIGHTNESS);
		//if ( SPEED != savedSpeed ) updateSettings_speed(SPEED);
		if ( PROGRAM != savedProgram ) updateSettings_program(PROGRAM);
		if ( MODE != savedMode ) updateSettings_mode(MODE);
	}
	
	if (!displayOn){
		FastLED.clear();
	}
	
	else {

		mappingOverride ? cMapping = cOverrideMapping : cMapping = defaultMapping;

		//PROFILE_START("pattern_render");
		switch(PROGRAM){

			case 0:
				defaultMapping = Mapping::TopDownProgressive;
				if (!rainbow::rainbowInstance) {
					rainbow::initRainbow(myXY);
				}
				rainbow::runRainbow();
				break; 

			case 1:
				// 1D; mapping not needed
				defaultMapping = Mapping::TopDownProgressive;
				if (!waves::wavesInstance) {
					waves::initWaves();
				}
				waves::runWaves(); 
				break;

			case 2:  
				defaultMapping = Mapping::TopDownSerpentine;
				if (!bubble::bubbleInstance) {
					bubble::initBubble(myXY);
				}
				bubble::runBubble();
				break;  

			case 3:
				defaultMapping = Mapping::TopDownProgressive;
				if (!dots::dotsInstance) {
					dots::initDots(myXY);
				}
				dots::runDots();
				break;  
			
			case 4:
				if (!fxWave2d::fxWave2dInstance) {
					fxWave2d::initFxWave2d(myXYmap, xyRect);
				}
				fxWave2d::runFxWave2d();
				break;

			case 5:    
				defaultMapping = Mapping::TopDownProgressive;
				if (!radii::radiiInstance) {
					radii::initRadii(myXY);
				}
				radii::runRadii();
				break;
			
			case 6:  
				if (!animartrix::animartrixInstance) {
					animartrix::initAnimartrix(myXYmap);
				}
				animartrix::runAnimartrix();
				break;

			case 7:    
				defaultMapping = Mapping::TopDownProgressive;
				if (!test::testInstance) {
					test::initTest(myXY);
				}
				test::runTest();
				break;

			case 8:    
				defaultMapping = Mapping::TopDownProgressive;
				if (!synaptide::synaptideInstance) {
					synaptide::initSynaptide(myXY);
				}
				synaptide::runSynaptide();
				break;

			case 9:    
				defaultMapping = Mapping::TopDownProgressive;
				if (!cube::cubeInstance) {
					cube::initCube(myXY);
				}
				cube::runCube();
				break;

			case 10:    
				defaultMapping = Mapping::TopDownProgressive;
				if (!horizons::horizonsInstance) {
					horizons::initHorizons(myXY);
				}
				horizons::runHorizons();
				break;
			
			case 11:    
				defaultMapping = Mapping::TopDownProgressive;
				if (!audioTest::audioTestInstance) {
					audioTest::initAudioTest(myXY);
				}
				audioTest::runAudioTest();
				break;
		}
		//PROFILE_END();
	}

	//PROFILE_START("FastLED.show");
	FastLED.show();
	//PROFILE_END();
	
	// upon BLE disconnect
	if (!deviceConnected && wasConnected) {
		if (debug) {Serial.println("Device disconnected.");}
		delay(500); // give the bluetooth stack the chance to get things ready
		pServer->startAdvertising();
		if (debug) {Serial.println("Start advertising");}
		wasConnected = false;
	}
	
} // loop()
