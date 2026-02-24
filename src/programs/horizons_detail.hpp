#include "reference\horizonPalettes.h"
#include "bleControl.h"

namespace horizons {

bool horizonsInstance = false;
uint16_t (*xyFunc)(uint8_t x, uint8_t y);
void initHorizons(uint16_t (*xy_func)(uint8_t, uint8_t)) {
	horizonsInstance = true;
	xyFunc = xy_func;
}

bool firstRun = true;
uint8_t cyclesPerPalette = 3;
uint8_t cycleCounter = 0;
uint32_t cycleDuration = 300000;


// ARTISCIC FACTORS ===============================================================================

extern const TProgmemRGBGradientPaletteRef hGradientPalettes[]; 
extern const uint8_t hGradientPaletteCount;

// Gradient modes for flexible gradient control
enum GradientMode {
	GRAD_EASE,       // Use FastLED ease functions
	GRAD_CUSTOM,
	GRAD_POWER,      // Power curve (exponential)
	GRAD_LINEAR      // Simple linear gradient
};

// For colorBoost()
EaseType sat_ease = EASE_IN_SINE;
EaseType lum_ease = EASE_NONE; // EASE_OUT_QUAD

// for cloud noise
uint8_t cloudScale = 75;
uint16_t cloudSpeed = 1000;

struct Scene {
	uint8_t lightBias;
	uint8_t dramaScale;
};

Scene scene;

uint8_t getFloorLow[11][11] {
	{10,12,14,16,18,20,22,24,26,28,30},
	{9,11,13,15,17,19,21,23,25,27,29},
	{8,10,12,14,16,18,20,22,24,26,28},
	{7,9,11,13,15,17,19,21,23,25,27},
	{6,8,10,12,14,16,18,20,22,24,26},
	{5,7,9,11,13,15,17,19,21,23,25},
	{4,6,8,10,12,14,16,18,20,22,24},
	{3,5,7,9,11,13,15,17,19,21,23},
	{2,4,6,8,10,12,14,16,18,20,22},
	{1,3,5,7,9,11,13,15,17,19,21},
	{0,2,4,6,8,10,12,14,16,18,20}
};

uint8_t getFloorHigh[11][11] {
	{15,17,19,21,23,25,28,31,34,37,40},
	{15,17,19,21,23,26,29,33,37,41,42},
	{15,17,20,22,24,27,31,34,38,42,44},
	{15,17,20,22,25,28,32,35,40,44,46},
	{15,18,21,23,26,29,33,37,41,45,48},
	{15,18,21,24,27,30,34,38,42,46,50},
	{16,19,22,25,28,31,36,39,43,48,52},
	{17,20,23,26,29,32,38,40,45,50,54},
	{18,21,24,27,30,33,40,41,47,52,56},
	{19,22,25,28,31,34,42,43,49,53,58},
	{20,23,26,29,32,35,40,45,50,55,60}
};

uint8_t getCeilingLow[11][11] {
	{80,90,100,110,120,130,140,150,160,170,180},
	{79,89,99,109,119,129,139,149,159,169,179},
	{78,88,98,108,118,128,138,148,158,168,178},
	{77,87,97,107,117,127,137,147,157,167,177},
	{76,86,96,106,116,126,136,146,156,166,176},
	{75,85,95,105,115,125,135,145,155,165,175},
	{74,84,94,104,114,124,134,144,154,164,174},
	{73,83,93,103,113,123,133,143,153,163,173},
	{72,82,92,102,112,122,132,142,152,162,172},
	{71,81,91,101,111,121,131,141,151,161,171},
	{70,80,90,100,110,120,130,140,150,160,170}
};

uint8_t getCeilingHigh[11][11] {
	{90,102,114,126,138,150,162,174,186,198,210},
	{92,104,116,128,140,152,164,176,188,200,212},
	{94,106,118,130,142,154,166,178,190,202,214},
	{96,108,120,132,144,156,168,180,192,204,216},
	{98,110,122,134,146,158,170,182,194,206,218},
	{100,112,124,136,148,160,172,184,196,208,220},
	{102,114,126,138,150,162,174,186,198,210,222},
	{104,116,128,140,152,164,176,188,200,212,224},
	{106,118,130,142,154,166,178,190,202,214,226},
	{108,120,132,144,156,168,180,192,204,216,228},
	{110,122,134,146,158,170,182,194,206,218,230}
};

// lightCycle timing
TimeRamp lightCycle{0, 0, 0};  // Initialize with placeholder values
RampPhase phase;               // Current phase of the light cycle
uint32_t risingTime;
uint32_t holdTime;
uint32_t fallingTime;
uint32_t cycleTime;
uint32_t cycleEnd;
uint32_t nextCycleDelay;
uint32_t nextCycleStart;
uint16_t currentAlpha;


// HELPER FUNCTIONS ========================================================================================

// Debug helpers - convert enums/indices to printable strings
const char* phaseToString(RampPhase p) {
	switch (p) {
		case RampPhase::Inactive: return "Inactive";
		case RampPhase::Rising:   return "Rising";
		case RampPhase::Plateau:  return "Plateau";
		case RampPhase::Falling:  return "Falling";
		default:                  return "Unknown";
	}
}

extern const char* paletteNames[];

const char* getPaletteName(uint8_t index) {
	if (index < hGradientPaletteCount) {
		return paletteNames[index];
	}
	return "Unknown";
}

// Noise animation speed: lower = slower, more gradual changes (typical range: 5-50)
uint16_t noiseSpeed = 50;  // Speed multiplier for Perlin noise time axis


using ::map;
// Helper function for 16-bit mapping (Arduino's map() but for uint16_t)
inline uint16_t map16(uint16_t x, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max) {
	return (uint16_t)map((int32_t)x, (int32_t)in_min, (int32_t)in_max, (int32_t)out_min, (int32_t)out_max);
}

// Convert 8-bit conceptual value (0-255) to 16-bit (0-65535) for internal math
inline uint16_t to16(uint8_t val8) {
	return ((uint16_t)val8 << 8) | val8;  // Spreads 0-255 to 0-65535 smoothly
}


// PANELS ==============================================================================================

uint8_t horizonOffset = 0; // + raises horizon; - lowers horizon
// Note: recycleTriggered is declared in bleControl.h (global scope)

enum Panel {
	UPPER = 0,
	LOWER
};	

struct panel {

	// Panel geometry
	Panel panelPosition;
	uint8_t panelHeight;
	uint8_t topRow;
	uint8_t bottomRow;

	// Gradient mapping - using 16-bit for smooth transitions
	// Values are in 0-65535 range, mapped to 0-255 palette index at final lookup
	uint16_t cGradientFloor;      // Current floor (animated)
	uint16_t cGradientCeiling;    // Current ceiling (animated)

	// Scenes
	Scene scene;
	uint8_t lightBias;
	uint8_t dramaScale;
	uint8_t lightIndex;
	uint8_t dramaIndex;

	// High/Low bounds for animation (16-bit, scaled from 8-bit conceptual values)
	uint16_t gradientFloorHighOld;
	uint16_t gradientFloorHighNew;
	uint16_t gradientFloorLowOld;
	uint16_t gradientFloorLowNew;
	uint16_t gradientCeilingHighOld;
	uint16_t gradientCeilingHighNew;
	uint16_t gradientCeilingLowOld;
	uint16_t gradientCeilingLowNew;

	// Gradient curve control
	GradientMode mode;         // Which curve algorithm to use
	EaseType easeType;         // For GRAD_EASE mode
	float curvePower;          // For GRAD_POWER mode (1.0 = linear, <1 = ease out, >1 = ease in)
		
	// Palette control
	uint8_t cPaletteNumber;
	CRGBPalette16 cPalette;
	
};

panel upper;
panel lower;

void setPanels() {
	upper.panelPosition = UPPER;
	upper.topRow = 0;
	upper.panelHeight = (HEIGHT / 2) + horizonOffset;
	upper.bottomRow = upper.panelHeight - 1;
	lower.panelPosition = LOWER;
	lower.topRow = upper.bottomRow + 1;
	lower.panelHeight = HEIGHT - upper.panelHeight;
	lower.bottomRow = HEIGHT - 1;
}

void initializePanel(panel& p) {

	p.gradientFloorHighOld = to16(40);
	p.gradientCeilingHighOld = to16(200);
	p.cGradientFloor = p.gradientFloorHighOld = p.gradientFloorHighNew = p.gradientFloorLowOld = p.gradientFloorLowNew;
	p.cGradientCeiling = p.gradientCeilingHighOld = p.gradientCeilingHighNew = p.gradientCeilingLowOld = p.gradientCeilingLowNew;
	p.mode = GRAD_EASE;
	p.scene.lightBias = p.scene.dramaScale = 0;  
	p.cPaletteNumber = 0;
	p.cPalette = gGradientPalettes[p.cPaletteNumber];
	
	switch(p.panelPosition) {
		case UPPER:
			p.easeType = EASE_IN_OUT_SINE;
			break;
		case LOWER:
			p.easeType = EASE_IN_QUAD;
			break;
	}
} // initializePanel

void saveNewToOld(panel& p) {
	p.gradientFloorHighOld = p.gradientFloorHighNew;
	p.gradientFloorLowOld = p.gradientFloorLowNew;
	p.gradientCeilingHighOld = p.gradientCeilingHighNew;
	p.gradientCeilingLowOld = p.gradientCeilingLowNew;
}

//LOOP CYCLE FUNCTIONS ================================================================================

extern void startNewCycle();
extern void restart();
extern void setGradients(panel& p);

void rotatePalette(panel& p) {
	p.cPaletteNumber = addmod8(p.cPaletteNumber, 1, hGradientPaletteCount);
	p.cPalette = hGradientPalettes[p.cPaletteNumber];
}

void applyTriggers(panel& p){

	if (p.panelPosition == UPPER && rotateUpperTriggered) {
		rotatePalette(p);
		rotateUpperTriggered = false;
	}
	if (p.panelPosition == LOWER && rotateLowerTriggered) {
		rotatePalette(p);
		rotateLowerTriggered = false;
	}
	if (restartTriggered) {
		restartTriggered = false;
		restart();
	}
	if (updateScene) {
		setGradients(upper);
		setGradients(lower);
		saveNewToOld(upper);
		saveNewToOld(lower);
		updateScene = false;
	}
}

void startLightCycle() {
	
	cycleCounter += 1;
	uint32_t now = fl::millis();

	switch(cycleDurationManualMode) {

		case true:
			cycleDuration = cCycleDuration * 1000;
			risingTime = fallingTime = (cycleDuration * 0.45); 
			holdTime = nextCycleDelay = (cycleDuration - (risingTime + fallingTime)) / 2; 		
			break;

		case false:
			float dramaFactor = map(upper.dramaIndex, 0, 10, 3, .5 );
			uint32_t baseRiseFall = 90000;  
			uint32_t basePause = 10000;
			risingTime = fallingTime = baseRiseFall * dramaFactor; 		
			holdTime = nextCycleDelay = basePause / dramaFactor;
			break;
	
	}

	cycleTime = risingTime + holdTime + fallingTime; 
	cycleEnd = now + cycleTime;
	nextCycleStart = cycleEnd + nextCycleDelay;

	lightCycle = TimeRamp(risingTime, holdTime, fallingTime);

	lightCycle.trigger(now);

}

void startNewCycle() {
	saveNewToOld(upper);
	saveNewToOld(lower);
	setGradients(upper);
	setGradients(lower);
	startLightCycle();
}

void restart(){
	rotatePalette(upper);
	rotatePalette(lower);
	cycleCounter = 0;
	startNewCycle();
}

void setInitialGradients(panel& p) {
	
	// Set initial "old" scene
	p.scene.lightBias = random(0,11);
	p.scene.dramaScale = random(0,11);
	p.lightIndex = p.scene.lightBias;
	p.dramaIndex = p.scene.dramaScale;
	
	// Set initial "old" starting gradients 
	p.gradientFloorHighOld = to16(getFloorHigh[p.lightIndex][p.dramaIndex]);
	p.gradientFloorLowOld = to16(getFloorLow[p.lightIndex][p.dramaIndex]);
	p.gradientCeilingHighOld = to16(getCeilingHigh[p.lightIndex][p.dramaIndex]);
	p.gradientCeilingLowOld = to16(getCeilingLow[p.lightIndex][p.dramaIndex]);

	// Set initial "new" scene
	p.scene.lightBias = random(0,11);
	p.scene.dramaScale = random(0,11);
	p.lightIndex = p.scene.lightBias;
	p.dramaIndex = p.scene.dramaScale;

	// Set initial "new" starting gradients
	p.gradientFloorHighNew = to16(getFloorHigh[p.lightIndex][p.dramaIndex]);
	p.gradientFloorLowNew = to16(getFloorLow[p.lightIndex][p.dramaIndex]);
	p.gradientCeilingHighNew = to16(getCeilingHigh[p.lightIndex][p.dramaIndex]);
	p.gradientCeilingLowNew = to16(getCeilingLow[p.lightIndex][p.dramaIndex]);

}

void setScene(panel& p) {
	if (sceneManualMode==true) {
		p.scene.lightBias  = cLightBias;
		p.scene.dramaScale = cDramaScale;
	} else {
		p.scene.lightBias  = random(0,11);
		p.scene.dramaScale = random(0,11);
	}
	p.lightIndex = p.scene.lightBias;
	p.dramaIndex = p.scene.dramaScale; 
}

void setGradients(panel& p) {
	setScene(p);
	p.gradientFloorHighNew = to16(getFloorHigh[p.lightIndex][p.dramaIndex]);
	p.gradientFloorLowNew = to16(getFloorLow[p.lightIndex][p.dramaIndex]);
	p.gradientCeilingHighNew = to16(getCeilingHigh[p.lightIndex][p.dramaIndex]);
	p.gradientCeilingLowNew = to16(getCeilingLow[p.lightIndex][p.dramaIndex]);
}


void checkTransitions() {
	uint32_t now = fl::millis();
	phase = lightCycle.getCurrentPhase(now);
	if (cycleCounter > cyclesPerPalette) {
		restart();
	} 
	if (now > nextCycleStart) {
		startNewCycle();
	}
}

void updateValues(panel& p) {

	uint32_t now = fl::millis();
	currentAlpha = lightCycle.update16(now);
	phase = lightCycle.getCurrentPhase(now);

	switch(phase) {
		case RampPhase::Rising: // darkening
			p.cGradientFloor = map(currentAlpha, 0, 65535, p.gradientFloorHighOld, p.gradientFloorLowOld);
			p.cGradientCeiling = map(currentAlpha, 0, 65535, p.gradientCeilingHighOld, p.gradientCeilingLowOld);
			break;
		case RampPhase::Plateau:
			// Hold at dark values
			p.cGradientFloor = p.gradientFloorLowOld;
			p.cGradientCeiling = p.gradientCeilingLowOld;
			break;
		case RampPhase::Falling: // lightening
			p.cGradientFloor = map(currentAlpha, 0, 65535, p.gradientFloorHighNew, p.gradientFloorLowOld);
			p.cGradientCeiling = map(currentAlpha, 0, 65535, p.gradientCeilingHighNew, p.gradientCeilingLowOld);
			break;
		case RampPhase::Inactive:
			// Hold at light values
			p.cGradientFloor = p.gradientFloorHighNew;
			p.cGradientCeiling = p.gradientCeilingHighNew;
			break;
		}

	} // updateValues()

// Get palette index for a given row in a panel using variable gradient system
// Returns 16-bit index (0-65535) for use with ColorFromPaletteExtended
uint16_t getPaletteIndex16(panel& p, uint8_t row) {

	// Normalize row position to 0.0-1.0
	float position = (float)(row - p.topRow) / (float)(p.panelHeight - 1);

	// Apply curve based on gradient mode
	float curved = position;
	switch (p.mode) {
		case GRAD_EASE: {
			// Use FastLED ease functions (fast, optimized)
			uint16_t pos16 = position * 65535.0f;
			uint16_t eased16 = ease16(p.easeType, pos16);
			curved = eased16 / 65535.0f;
			break;
		}
		case GRAD_POWER:
			// Power curve (exponential)
			curved = fl::powf(position, p.curvePower);
			break;
		case GRAD_CUSTOM: {
			//getControlPoints(controlPoints);
			//curved = cubicBezier(controlPoints, position);
			break;
		}
		case GRAD_LINEAR:
		default:
			// No curve, just use position as-is
			curved = position;
			break;
	}

	// Map curved value to palette index range; 16-bit (0-65535) for full precision
	uint16_t index;
	switch(p.panelPosition) {
		case UPPER:
			// Upper panel: ceiling at top (row 0), floor at bottom (horizon)
			index = p.cGradientCeiling - (uint16_t)((float)(p.cGradientCeiling - p.cGradientFloor) * curved);
			break;
		case LOWER:
		default:
			// Lower panel: floor at top (horizon), ceiling at bottom
			index = p.cGradientFloor + (uint16_t)((float)(p.cGradientCeiling - p.cGradientFloor) * curved);
			break;
	}

	return index;

} //getPaletteIndex16()

void renderColors(panel& p) {
	for (uint8_t y = p.topRow; y <= p.bottomRow; y++) {
		uint16_t paletteIndex = getPaletteIndex16(p, y);
		CRGB color = ColorFromPaletteExtended(p.cPalette, paletteIndex, 255, LINEARBLEND_NOWRAP);
		for (uint8_t x = 0; x < WIDTH; x++) {
			leds[xyFunc(x, y)] = color;
		}
	}
}

void addTexture() {
	for (uint16_t i = 0; i < NUM_LEDS; i++) {
		// Get LED x,y position
		uint8_t x = i % WIDTH;
		uint8_t y = i / WIDTH;

		// 2D Perlin noise with slow time evolution
		// Scale creates patch size, time creates movement
		uint16_t noiseValue = inoise8(x * cloudScale, y * cloudScale, fl::millis() / cloudSpeed);

		// Map noise to subtle darkening (e.g., 85-100% brightness)
		uint8_t dimFactor = map(noiseValue, 0, 255, 215, 255);  

		leds[i].nscale8(dimFactor);
	}
}

void addSaturation() {
	for (uint16_t i = 0; i < NUM_LEDS; i++) {
		CRGB original_color = leds[i];
		leds[i] = original_color.colorBoost(sat_ease, lum_ease);
	}
}


// ========================================================================================

void runHorizons() {

	if (firstRun) {
		setPanels();
		initializePanel(upper);
		initializePanel(lower);
		upper.cPaletteNumber = random(0,hGradientPaletteCount-1);
		upper.cPalette = hGradientPalettes[upper.cPaletteNumber];
		lower.cPaletteNumber = addmod8(upper.cPaletteNumber, random(1,3), hGradientPaletteCount);
		lower.cPalette = hGradientPalettes[lower.cPaletteNumber];
		cycleCounter = 0;
		setInitialGradients(upper);
		setInitialGradients(lower);
		startLightCycle();
		firstRun = false;
	}

	applyTriggers(upper);
	applyTriggers(lower);
	
	checkTransitions();
	
	updateValues(upper);
	updateValues(lower);

	renderColors(upper);
	renderColors(lower);

	EVERY_N_SECONDS(5) {
		if (debug) {
			uint32_t now = fl::millis();
			uint16_t debugAlpha = lightCycle.update16(now);
			uint8_t phaseProgress = (100UL * debugAlpha) / 65535;
			RampPhase debugPhase = lightCycle.getCurrentPhase(now);
			if (debugPhase == RampPhase::Falling) { phaseProgress = 100-phaseProgress; }
			Serial.println("-----------------------");
			Serial.print("Phase: "); Serial.println(phaseToString(debugPhase));
			Serial.print("Phase progress: "); Serial.print(phaseProgress); Serial.println("%");
			Serial.print("Upper palette: "); Serial.println(getPaletteName(upper.cPaletteNumber));
			Serial.print("Lower palette: "); Serial.println(getPaletteName(lower.cPaletteNumber));
			Serial.print("Cycle counter: "); Serial.println(cycleCounter);
			Serial.print("Upper lightIndex: "); Serial.println(upper.lightIndex);
			Serial.print("Upper dramaIndex: "); Serial.println(upper.dramaIndex);
			Serial.print("Lower lightIndex: "); Serial.println(lower.lightIndex);
			Serial.print("Lower dramaIndex: "); Serial.println(lower.dramaIndex);
		}
	}

	//addTexture();
	addSaturation();

}

} // namespace horizons