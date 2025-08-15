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

#define BIG_BOARD
//#undef BIG_BOARD

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
  	//#define POWER_MILLIAMPS 5000
#else 
	#include "matrixMap_24x24.h"
	#define HEIGHT 24 
    #define WIDTH 24
    #define NUM_SEGMENTS 1
    #define NUM_LEDS_PER_SEGMENT 576
	#define POWER_MILLIAMPS 1000
#endif

//#define POWER_LIMITER_ACTIVE
//#define POWER_VOLTS 5

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

#include "myAnimartrix.hpp"
bool animartrixFirstRun = true;

#include "bubble.hpp"

#include "radii.hpp"

#include "dots.hpp"

#include "fxWaves2d.hpp"

#include "waves.hpp"

#include "rainbow.hpp"

// Misc global variables ********************************************************************

// Variables moved to waves namespace (were: blendFract, hueIncMax, newcolor)

uint8_t savedProgram;
//uint8_t savedMode;
uint8_t savedSpeed;
uint8_t savedBrightness;

// SECONDS_PER_PALETTE moved to waves namespace

// MAPPINGS **********************************************************************************

extern const uint16_t loc2indSerpByRow[HEIGHT][WIDTH] PROGMEM;
extern const uint16_t loc2indProgByRow[HEIGHT][WIDTH] PROGMEM;
extern const uint16_t loc2indSerp[NUM_LEDS] PROGMEM;
extern const uint16_t loc2indProg[NUM_LEDS] PROGMEM;
extern const uint16_t loc2indProgByColBottomUp[WIDTH][HEIGHT] PROGMEM;

uint16_t XY(uint8_t x, uint8_t y) {
	//if (x >= WIDTH || y >= HEIGHT) return 0;
	return loc2indProgByRow[y][x];
}

/*
uint16_t dotsXY(uint8_t x, uint8_t y) { 
	if (x >= WIDTH || y >= HEIGHT) return 0;
	//uint8_t yFlip = (HEIGHT - 1) - y ;       // comment/uncomment to flip direction of vertical motion
	return loc2indProgByColBottomUp[x][y];
}
*/

// For XYMap custom mapping


uint16_t myXYFunction(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
		width = WIDTH;
		height = HEIGHT;
		if (x >= width || y >= height) return 0;
		ledNum = loc2indProgByRow[y][x];
		return ledNum;
}


uint16_t myXYFunction(uint16_t x, uint16_t y, uint16_t width, uint16_t height);

XYMap myXYmap = XYMap::constructWithUserFunction(WIDTH, HEIGHT, myXYFunction);
//XYMap myXYmap = XYMap::constructWithLookUpTable(WIDTH, HEIGHT, loc2indProg);
XYMap xyRect = XYMap::constructRectangularGrid(WIDTH, HEIGHT);

//********************************************************************************************

void setup() {
		
		preferences.begin("settings", true); // true == read only mode
			savedProgram  = preferences.getUChar("program");
			//savedMode  = preferences.getUChar("mode");
			savedBrightness  = preferences.getUChar("brightness");
			savedSpeed  = preferences.getUChar("speed");
		preferences.end();	

		//PROGRAM = 1;
		MODE = 0;
		//BRIGHTNESS = 155;
		// SPEED = 5;
		PROGRAM = savedProgram;
		//MODE = savedMode;
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

/*void updateSettings_mode(uint8_t newMode){
 preferences.begin("settings",false);  // false == read write mode
	 preferences.putUChar("mode", newMode);
 preferences.end();
 savedMode = newMode;
 if (debug) {Serial.println("Mode setting updated");}
}*/

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

// *************************************************************************************************************************
// RAINBOW MATRIX **********************************************************************************************************
// Code matrix format: 2D, needs loc2indSerpByRow for 22x22;
// Moved to rainbow.hpp and rainbow_detail.hpp

/*
void DrawOneFrame( uint8_t startHue8, int8_t yHueDelta8, int8_t xHueDelta8) {
	uint8_t lineStartHue = startHue8;
	for( uint8_t y = 0; y < HEIGHT; y++) {
		lineStartHue += yHueDelta8;
		uint8_t pixelHue = lineStartHue;      
		for( uint8_t x = 0; x < WIDTH; x++) {
			pixelHue += xHueDelta8;
			
			switch (mapping) {
 				case 1:   
					ledNum = loc2indProgByRow[y][x];   
					break;
				case 2:
					ledNum = loc2indSerpByRow[y][x];
					break;
				}
			
			leds[ledNum] = CHSV(pixelHue, 255, 255);
		}  
	}
}

void rainbowMatrix () {
		uint32_t ms = millis();
		int32_t yHueDelta32 = ((int32_t)cos16( ms * (27/1) ) * (350 / WIDTH));
		int32_t xHueDelta32 = ((int32_t)cos16( ms * (39/1) ) * (310 / HEIGHT));
		DrawOneFrame( ms / 65536, yHueDelta32 / 32768, xHueDelta32 / 32768);
 }
*/

// PRIDE/WAVES**************************************************************************************************************
// Code matrix format: 1D, Serpentine
// Moved to waves.hpp and waves_detail.hpp

/*
void prideWaves() {

	if (MODE==0 && rotateWaves) {
		EVERY_N_SECONDS( SECONDS_PER_PALETTE ) {
			//capture the prior target palNum as the current palNum 
			gCurrentPaletteNumber = gTargetPaletteNumber; 
			//then set a new target
			gTargetPaletteNumber = addmod8( gTargetPaletteNumber, 1, gGradientPaletteCount);
			gTargetPalette = gGradientPalettes[ gTargetPaletteNumber ];
			if (debug) {
				Serial.print("Next color palette number: ");
				Serial.println(gTargetPaletteNumber);
			}
		}
	}
	
	EVERY_N_MILLISECONDS(40) {
		nblendPaletteTowardPalette( gCurrentPalette, gTargetPalette, 16); 
	}
	
	switch(MODE){
		case 0: hueIncMax = 2800; break;
		case 1: hueIncMax = 300; break;
	}

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

		if (MODE==0) {
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
	
		switch (MODE) {
	
			case 0: {
				uint8_t index = hue8;
				index = scale8( index, 240);
				newcolor = ColorFromPalette( gCurrentPalette, index, bri8);
				blendFract = 93;
				break;
			}
			
			case 1: {
				newcolor = CHSV( hue8, sat8, bri8);
				blendFract = 21;
				break;
			}
		}
				
		uint16_t pixelnumber = i;
		//pixelnumber = (NUM_LEDS-1) - pixelnumber;  // comment/uncomment this line to reverse apparent direction of LED progression   
	
		switch (mapping) {
	
			case 1:   
				ledNum = loc2indProg[pixelnumber];   
				break;
			
			case 2:
				ledNum =loc2indSerp[pixelnumber];
				break;
		
			}
	
	 	nblend( leds[ledNum], newcolor, blendFract);

	}

}
*/

// *************************************************************************************************************************
// SOAP BUBBLE**************************************************************************************************************
// Source: Stepko; his grid 

/*
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

//*******************************************************************************************

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

//*******************************************************************************************

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

//*******************************************************************************************

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
				leds[XY(i, j)] = CHSV(~noise3d[i][j]*3,255,BRIGHTNESS);
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
*/

// *************************************************************************************************************************** 
// DOT DANCE *****************************************************************************************************************
// Moved to dots.hpp and dots_detail.hpp

/*
//FL::FX - none

byte osci[4]; 
byte pX[4];
byte pY[4];

void PixelA(uint8_t x, uint8_t y, byte color) {
	leds[XY(x, y)] = CHSV(color, 255, 255);
}

void PixelB(uint8_t x, uint8_t y, byte color) {
	leds[XY(x, y)] = CHSV(color, 255, 255);
}

// set the speeds (and by that ratios) of the oscillators here
// JSH add: set separate x and y oscillator arrays 
void MoveOscillators() {
	osci[0] = osci[0] + 5;
	osci[1] = osci[1] + 2;
	osci[2] = osci[2] + 3;
	osci[3] = osci[3] + 4;
	for(int i = 0; i < 4; i++) { 
		pX[i] = map(sin8(osci[i]),0,255,0,WIDTH-1);
		pY[i] = map(sin8(osci[i]),0,255,0,HEIGHT-1);
	}
}

// give it a linear tail downwards
void VerticalStream(byte scale)  
{
	for(uint8_t x = 0; x < WIDTH ; x++) {
		for(uint8_t y = 1; y < HEIGHT; y++) { 
			leds[XY(x,y)] += leds[XY(x,y-1)];
			leds[XY(x,y)].nscale8(scale);
		}
	}
	for(uint8_t x = 0; x < WIDTH; x++) 
		leds[XY(x,0)].nscale8(scale);
}

//*****************************************************************************

void dotDance() {

	MoveOscillators();

	PixelA( 
		(pX[2]+pX[0]+pX[1])/3,
		(pY[1]+pY[3]+pY[2])/3,
		osci[1]
	);

	PixelB( 
		(pX[3]+pX[0]+pX[1])/3,
		(pY[1]+pY[0]+pY[2])/3,
		osci[3]
	);
	 
	VerticalStream(125);
	FastLED.delay(10);
 
}
*/

// **************************************************************************************************************************
// fxWave2d *****************************************************************************************************************
// Moved to fxWaves2d.hpp and fxWaves2d_detail.hpp

/*
bool firstWave = true;

bool autoTrigger = true;
bool easeModeSqrt = false;

float triggerSpeed = .4f;
uint8_t blurAmount = 0;
uint8_t blurPasses = 1;
float superSample = 1.f;

float speedLower = .16f;
float dampeningLower = 8.0f;
bool halfDuplexLower = true;
uint8_t blurAmountLower = 0;
uint8_t blurPassesLower = 1;

float speedUpper = .24f;
float dampeningUpper = 6.0f;
bool halfDuplexUpper = true;
uint8_t blurAmountUpper = 12;
uint8_t blurPassesUpper = 1;

uint32_t fancySpeed = 796;
uint8_t fancyIntensity = 32;
float fancyParticleSpan = .01f;

//****************************************************************************

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

//****************************************************************************

WaveFx::Args CreateArgsLower() {
		WaveFx::Args out;
		out.factor = SuperSample::SUPER_SAMPLE_4X;
		out.half_duplex = true;
		out.auto_updates = true;  
		out.speed = 0.26f; 
		out.dampening = 7.0f;
		out.crgbMap = fl::make_shared<WaveCrgbGradientMap>(electricBlueFirePal);  
		return out;
}  

WaveFx::Args CreateArgsUpper() {
		WaveFx::Args out;
		out.factor = SuperSample::SUPER_SAMPLE_4X; 
		out.half_duplex = true;  
		out.auto_updates = true; 
		out.speed = 0.14f;      
		out.dampening = 6.0f;     
		out.crgbMap = fl::make_shared<WaveCrgbGradientMap>(electricGreenFirePal); 
		return out;
}

// For screenTest, these need to use xyRect
// For LED panel display, these need to use myXYmap
WaveFx waveFxLower(myXYmap, CreateArgsLower());
WaveFx waveFxUpper(myXYmap, CreateArgsUpper()); 

// For screenTest, this needs to use myXYmap
// For LED panel display, this needs to use xyRect
Blend2d fxBlend(xyRect);

//*************************************************************************

SuperSample getSuperSample() {
		switch (int(superSample)) {
		case 0:		return SuperSample::SUPER_SAMPLE_NONE;
		case 1: 	return SuperSample::SUPER_SAMPLE_2X;
		case 2:		return SuperSample::SUPER_SAMPLE_4X;
		case 3: 	return SuperSample::SUPER_SAMPLE_8X;
		default:	return SuperSample::SUPER_SAMPLE_NONE;
		}
}

//*************************************************************************

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

} // triggerRipple()

//*************************************************************************

void applyFancyEffect(uint32_t now, bool button_active) {
	 
		uint32_t total = map(fancySpeed, 0, 1000, 1000, 100);
		
		static TimeRamp pointTransition = TimeRamp(total, 0, 0);

		if (button_active) {
			pointTransition.trigger(now, total, 0, 0);
			fancyTrigger = false;
			if (debug) Serial.println("Fancy trigger applied");
		}

		if (!pointTransition.isActive(now)) {
			return;
		}

		int mid_x = WIDTH / 2;
		int mid_y = HEIGHT / 2;
		
		//int MAX_DIMENSION = MAX(WIDTH, HEIGHT);
		int amount = MAX_DIMENSION / 2;
		
		// Calculate the outermost edges for the cross (may be outside of actual matrix??)
		int edgeLeft = mid_x - amount;  
		int edgeRight = mid_x + amount;  
		int edgeBottom = mid_y - amount;
		int edgeTop = mid_y + amount;
		
		int curr_alpha = pointTransition.update8(now);
		
		// Map the animation progress to the four points of the expanding cross
    	// As curr_alpha increases from 0 to 255, these points move from center to edges
		int left_x = map(curr_alpha, 0, 255, mid_x, edgeLeft);  
		int right_x = map(curr_alpha, 0, 255, mid_x, edgeRight);  
		int down_y = map(curr_alpha, 0, 255, mid_y, edgeBottom);
		int up_y = map(curr_alpha, 0, 255, mid_y, edgeTop);

		 // Convert the 0-255 alpha to 0.0-1.0 range
		float curr_alpha_f = curr_alpha / 255.0f;
    
		// Calculate the wave height value - starts high and decreases as animation progresses
    	// This makes the waves stronger at the beginning of the animation
		float valuef = (1.0f - curr_alpha_f) * fancyIntensity/ 255.0f;

		// Thickness of the cross lines
		int span = MAX(fancyParticleSpan * MAX_DIMENSION, 1) ;
 
    	// Add wave energy along the four expanding lines of the cross
    	// Each line is a horizontal or vertical span of pixels

		// Left-moving horizontal line
		for (int x = left_x - span; x < left_x + span; x++) {
			waveFxLower.addf(x, mid_y, valuef); 
			waveFxUpper.addf(x, mid_y, valuef); 
		}

		// Right-moving horizontal line
		for (int x = right_x - span; x < right_x + span; x++) {
			waveFxLower.addf(x, mid_y, valuef);
			waveFxUpper.addf(x, mid_y, valuef);
		}

		// Downward-moving vertical line
		for (int y = down_y - span; y < down_y + span; y++) {
			waveFxLower.addf(mid_x, y, valuef);
			waveFxUpper.addf(mid_x, y, valuef);
		}

		// Upward-moving vertical line
		for (int y = up_y - span; y < up_y + span; y++) {
			waveFxLower.addf(mid_x, y, valuef);
			waveFxUpper.addf(mid_x, y, valuef);
		}
} // applyFancyEffect()

//*************************************************************************

void waveConfig() {
		
		U8EasingFunction easeMode = easeModeSqrt
			? U8EasingFunction::WAVE_U8_MODE_SQRT
			: U8EasingFunction::WAVE_U8_MODE_LINEAR;
		
		waveFxLower.setSpeed( speedLower * cRatDiff );             
		waveFxLower.setDampening( dampeningLower * cOffBase );      
		waveFxLower.setHalfDuplex(halfDuplexLower);    
		waveFxLower.setSuperSample(getSuperSample());  
		waveFxLower.setEasingMode(easeMode);           
		
		waveFxUpper.setSpeed(speedUpper * cOffDiff);             
		waveFxUpper.setDampening(dampeningUpper * cZ);     
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

} // waveConfig()

//*************************************************************************

void processAutoTrigger(uint32_t now) {
	
		static uint32_t nextTrigger = 0;
		
		uint32_t trigger_delta = nextTrigger - now;
		
		if (trigger_delta > 10000) {
				trigger_delta = 0;
		}
		
		if (autoTrigger) {
				if (now >= nextTrigger) {
						triggerRipple();
	
						float speed = 1.0f - triggerSpeed;;
						uint32_t min_rand = 350 * speed; 
						uint32_t max_rand = 2500 * speed; 

						uint32_t min = MIN(min_rand, max_rand);
						uint32_t max = MAX(min_rand, max_rand);
						
						if (min == max) {
								max += 1;
						}
						
						nextTrigger = now + random(min, max);
				}
		}
} // processAutoTrigger()

//**************************************************************************

void fxWave2d() {

	if (firstWave) {
		fxBlend.add(waveFxLower);
		fxBlend.add(waveFxUpper);
		waveConfig();
		firstWave = false;
	}
	
	uint32_t now = millis();
	EVERY_N_MILLISECONDS_RANDOM (2000,7000) { fancyTrigger = true; }
	applyFancyEffect(now, fancyTrigger);
	processAutoTrigger(now);
	Fx::DrawContext ctx(now, leds);
	fxBlend.draw(ctx);

} // fxWave2d()
*/

// **************************************************************************************************************************
// RADII ******************************************************************************************************************** 
// Moved to radii.hpp and radii_detail.hpp

/*
#define legs 3
bool setupm = 1;
const uint8_t C_X = WIDTH / 2;
const uint8_t C_Y = HEIGHT / 2;
const uint8_t mapp = 255 / MAX_DIMENSION;
struct {
	uint8_t angle;
	uint8_t radius;
}
rMap[WIDTH][HEIGHT];

void radii() {

	#define petals 5

	FastLED.clear();

	if (setupm) {
		setupm = 0;
		for (int8_t x = -C_X; x < C_X + (WIDTH % 2); x++) {
			for (int8_t y = -C_Y; y < C_Y + (HEIGHT % 2); y++) {
				rMap[x + C_X][y + C_Y].angle = 128 * (atan2(y, x) / PI);
				rMap[x + C_X][y + C_Y].radius = hypot(x, y) * mapp; //thanks Sutaburosu
			}
		}
	}

	static byte speed = 4;
	static uint16_t t;
	t += speed;

	for (uint8_t x = 0; x < WIDTH; x++) {
		for (uint8_t y = 0; y < HEIGHT; y++) {
			byte angle = rMap[x][y].angle;
			byte radius = rMap[x][y].radius;

			switch (MODE) {
	
				case 0: // octopus
					leds[XY(x, y)] = CHSV(t / 2 - radius, 255, sin8(sin8((angle * 4 - radius) / 4 + t) + radius - t * 2 + angle * legs));  
					break;
	
				case 1: // flower
					leds[XY(x, y)] = CHSV(t + radius, 255, sin8(sin8(t + angle * 5 + radius) + t * 4 + sin8(t * 4 - radius) + angle * 5));
					break;

				case 2: // lotus
					leds[XY(x, y)] = CHSV(248,181,sin8(t-radius+sin8(t + angle*petals)/5));
					break;
	
				case 3:  // radial waves
					leds[XY(x, y)] = CHSV(t + radius, 255, sin8(t * 4 + sin8(t * 4 - radius) + angle * 3));
					break;

			}
		}
	}
	
	FastLED.delay(15);

} // radii()
*/


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
			//if ( MODE != savedMode ) updateSettings_mode(MODE);
		}
 
		if (!displayOn){
			FastLED.clear();
		}
		
		else {
			
			switch(PROGRAM){

				case 0:  
					if (!rainbow::rainbowInstance) {
						rainbow::initRainbow();
					}
					rainbow::runRainbow();
					nscale8(leds,NUM_LEDS,BRIGHTNESS);
					break; 

				case 1:
					if (!waves::wavesInstance) {
						waves::initWaves();
					}
					waves::runWaves(); 
					break;
 
				case 2:  
					if (!bubble::bubbleInstance) {
						bubble::initBubble(WIDTH, HEIGHT, leds, XY);
					}
					bubble::runBubble();
					break;  

				case 3:  
					if (!dots::dotsInstance) {
						dots::initDots(XY);
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
					if (!radii::radiiInstance) {
						radii::initRadii(XY);
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
	
		// BLE CONTROL....

			// while connected
			/*if (deviceConnected) {
				if (brightnessChanged) { 
					pBrightnessCharacteristic->notify();
					brightnessChanged = false;
				}
			}*/

			// upon disconnect
			if (!deviceConnected && wasConnected) {
				if (debug) {Serial.println("Device disconnected.");}
				delay(500); // give the bluetooth stack the chance to get things ready
				pServer->startAdvertising();
				if (debug) {Serial.println("Start advertising");}
				wasConnected = false;
			}

		// ..................

} // loop()