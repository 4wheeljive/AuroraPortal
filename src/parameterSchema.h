#pragma once

#include "FastLED.h"
#include <ArduinoJson.h>
#include <string>

bool displayOn = true;

extern uint8_t PROGRAM;
extern uint8_t MODE;


// PROGRAM/MODE FRAMEWORK ****************************************

  enum Program : uint8_t {
      RAINBOW = 0,
      WAVES = 1,
      BUBBLE = 2,
      DOTS = 3,
      FXWAVE2D = 4,
      RADII = 5,
      ANIMARTRIX = 6,
      TEST = 7,
      SYNAPTIDE = 8,
      CUBE = 9,
      HORIZONS = 10,
      AUDIOTEST = 11,
      PROGRAM_COUNT
  };

  // Program names in PROGMEM
  const char rainbow_str[] PROGMEM = "rainbow";
  const char waves_str[] PROGMEM = "waves";
  const char bubble_str[] PROGMEM = "bubble";
  const char dots_str[] PROGMEM = "dots";
  const char fxwave2d_str[] PROGMEM = "fxwave2d";
  const char radii_str[] PROGMEM = "radii";
  const char animartrix_str[] PROGMEM = "animartrix";
  const char test_str[] PROGMEM = "test";
  const char synaptide_str[] PROGMEM = "synaptide";
  const char cube_str[] PROGMEM = "cube";
  const char horizons_str[] PROGMEM = "horizons";
  const char audiotest_str[] PROGMEM = "audiotest";

  const char* const PROGRAM_NAMES[] PROGMEM = {
      rainbow_str, waves_str, bubble_str, dots_str,
      fxwave2d_str, radii_str, animartrix_str, test_str,
      synaptide_str, cube_str, horizons_str, audiotest_str,
  };

  // Mode names in PROGMEM
   const char palette_str[] PROGMEM = "palette";
   const char pride_str[] PROGMEM = "pride";
   const char octopus_str[] PROGMEM = "octopus";
   const char flower_str[] PROGMEM = "flower";
   const char lotus_str[] PROGMEM = "lotus";
   const char radial_str[] PROGMEM = "radial";
   const char lollipop_str[] PROGMEM = "lollipop";
   const char polarwaves_str[] PROGMEM = "polarwaves";
   const char spiralus_str[] PROGMEM = "spiralus";
   const char caleido1_str[] PROGMEM = "caleido1";
   const char coolwaves_str[] PROGMEM = "coolwaves";
   const char chasingspirals_str[] PROGMEM = "chasingspirals";
   const char complexkaleido6_str[] PROGMEM = "complexkaleido6";
   const char water_str[] PROGMEM = "water";
   const char experiment1_str[] PROGMEM = "experiment1";
   const char experiment2_str[] PROGMEM = "experiment2";
   const char fluffyblobs_str[] PROGMEM = "fluffyblobs";
   const char test3_str[] PROGMEM = "test3";
   const char spectrumbars_str[] PROGMEM = "spectrumbars";
   const char vumeter_str[] PROGMEM = "vumeter";
   const char beatpulse_str[] PROGMEM = "beatpulse";
   const char bassripple_str[] PROGMEM = "bassripple";
   const char latencytest_str[] PROGMEM = "latencytest";
   const char radialspectrum_str[] PROGMEM = "radialspectrum";
   const char waveform_str[] PROGMEM = "waveform";
   const char spectrogram_str[] PROGMEM = "spectrogram";
   const char finespectrum_str[] PROGMEM = "finespectrum";
   const char busbeats_str[] PROGMEM = "busbeats";
   
   const char* const WAVES_MODES[] PROGMEM = {
         palette_str, pride_str
      };
   const char* const RADII_MODES[] PROGMEM = {
         octopus_str, flower_str, lotus_str, radial_str, lollipop_str
   };
   const char* const ANIMARTRIX_MODES[] PROGMEM = {
         polarwaves_str, spiralus_str, caleido1_str, coolwaves_str, chasingspirals_str,
         complexkaleido6_str, water_str, experiment1_str, experiment2_str, 
         fluffyblobs_str, test3_str 
      };
   const char* const AUDIOTEST_MODES[] PROGMEM = {
         spectrumbars_str, vumeter_str, beatpulse_str, bassripple_str, latencytest_str,
         radialspectrum_str, waveform_str, spectrogram_str, finespectrum_str, busbeats_str
      };
   
   const uint8_t MODE_COUNTS[] = {0, 2, 0, 0, 0, 5, 10, 0, 0, 0, 0, 11};

   // Visualizer parameter mappings - PROGMEM arrays for memory efficiency
   // Individual parameter arrays for each visualizer
   const char* const RAINBOW_PARAMS[] PROGMEM = {};
   const char* const WAVES_PALETTE_PARAMS[] PROGMEM = {"speed", "hueIncMax", "blendFract", "brightTheta"};
   const char* const WAVES_PRIDE_PARAMS[] PROGMEM = {"speed", "hueIncMax", "blendFract", "brightTheta"};
   const char* const BUBBLE_PARAMS[] PROGMEM = {"speed", "scale", "movement"};
   const char* const DOTS_PARAMS[] PROGMEM = {"tail"};
   const char* const FXWAVE2D_PARAMS[] PROGMEM = {"speed", "speedLower", "dampLower", "speedUpper", "dampUpper", "blurGlobFact"};
   const char* const RADII_OCTOPUS_PARAMS[] PROGMEM = {"zoom", "angle", "speedInt"};
   const char* const RADII_FLOWER_PARAMS[] PROGMEM = {"zoom", "angle", "speedInt"};
   const char* const RADII_LOTUS_PARAMS[] PROGMEM = {"zoom", "angle", "speedInt"};
   const char* const RADII_RADIAL_PARAMS[] PROGMEM = {"zoom", "angle", "speedInt"};
   const char* const RADII_LOLLIPOP_PARAMS[] PROGMEM = {"zoom", "angle", "speedInt", "radius", "edge"};
   const char* const ANIMARTRIX_POLARWAVES_PARAMS[] PROGMEM = {"speed", "zoom", "scale", "angle", "twist", "radius", "edge", "z", "ratBase", "ratDiff"};
   const char* const ANIMARTRIX_SPIRALUS_PARAMS[] PROGMEM = {"speed", "zoom", "scale", "angle", "z", "ratBase", "ratDiff", "offBase", "offDiff"};
   const char* const ANIMARTRIX_CALEIDO1_PARAMS[] PROGMEM = {"speed", "zoom", "scale", "angle", "z", "ratBase", "ratDiff", "offBase", "offDiff"};
   const char* const ANIMARTRIX_COOLWAVES_PARAMS[] PROGMEM = {"speed", "zoom", "scale", "angle", "z", "ratBase", "ratDiff", "offBase", "offDiff"};
   const char* const ANIMARTRIX_CHASINGSPIRALS_PARAMS[] PROGMEM = {"speed", "zoom", "scale", "angle", "twist", "radius", "edge", "ratBase", "ratDiff", "offBase", "offDiff"};
   const char* const ANIMARTRIX_COMPLEXKALEIDO6_PARAMS[] PROGMEM = {"starParamSet", "speed", "zoom", "scale", "angle", "twist", "radius", "radialSpeed", "edge", "z", "ratBase", "ratDiff"};
   const char* const ANIMARTRIX_WATER_PARAMS[] PROGMEM = {"speed", "zoom", "scale", "angle", "z", "ratBase", "ratDiff"};
   const char* const ANIMARTRIX_EXPERIMENT1_PARAMS[] PROGMEM = {"speed", "zoom", "scale", "angle", "z", "ratBase", "ratDiff"};
   const char* const ANIMARTRIX_EXPERIMENT2_PARAMS[] PROGMEM = {"speed", "zoom", "scale", "angle", "z", "ratBase", "ratDiff", "offBase", "offDiff"};
   const char* const ANIMARTRIX_FLUFFYBLOBS_PARAMS[] PROGMEM = {"speed", "zoom", "scale", "angle", "z", "radialSpeed", "linearSpeed", "z", "ratBase", "ratDiff" };
   const char* const ANIMARTRIX_TEST3_PARAMS[] PROGMEM = {"speed", "zoom", "scale", "angle", "z", "ratBase", "ratDiff", "offBase", "offDiff"};
   const char* const SYNAPTIDE_PARAMS[] PROGMEM = {"synSpeed", "bloomEdge", "decayBase", "decayChaos", "ignitionBase", "ignitionChaos", "neighborBase", "neighborChaos", "spatialDecay", "decayZones", "timeDrift", "pulse", "influenceBase", "influenceChaos", "entropyRate", "entropyBase", "entropyChaos"};
   const char* const CUBE_PARAMS[] PROGMEM = {"scale", "angleRateX", "angleRateY", "angleRateZ"};
   const char* const HORIZONS_PARAMS[] PROGMEM = {"lightBias", "dramaScale", "cycleDuration"};
   const char* const AUDIOTEST_SPECTRUMBARS_PARAMS[] PROGMEM = {};
   const char* const AUDIOTEST_VUMETER_PARAMS[] PROGMEM = {};
   const char* const AUDIOTEST_BEATPULSE_PARAMS[] PROGMEM = {};
   const char* const AUDIOTEST_BASSRIPPLE_PARAMS[] PROGMEM = {};
   const char* const AUDIOTEST_LATENCYTEST_PARAMS[] PROGMEM = {};
   const char* const AUDIOTEST_RADIALSPECTRUM_PARAMS[] PROGMEM = {};
   const char* const AUDIOTEST_WAVEFORM_PARAMS[] PROGMEM = {};
   const char* const AUDIOTEST_SPECTROGRAM_PARAMS[] PROGMEM = {};
   const char* const AUDIOTEST_FINESPECTRUM_PARAMS[] PROGMEM = {};
   const char* const AUDIOTEST_BUSBEATS_PARAMS[] PROGMEM = {};
   

   // Struct to hold visualizer name and parameter array reference
   struct VisualizerParamEntry {
      const char* visualizerName;
      const char* const* params;
      uint8_t count;
   };

   // String-based lookup table - mirrors JavaScript VISUALIZER_PARAMS
   // Can number values be replace by an array element count?
   const VisualizerParamEntry VISUALIZER_PARAM_LOOKUP[] PROGMEM = {
      {"rainbow", RAINBOW_PARAMS, 0},
      {"waves-palette", WAVES_PALETTE_PARAMS, 4},
      {"waves-pride", WAVES_PRIDE_PARAMS, 4},
      {"bubble", BUBBLE_PARAMS, 3},
      {"dots", DOTS_PARAMS, 1},
      {"fxwave2d", FXWAVE2D_PARAMS, 6},
      {"radii-octopus", RADII_OCTOPUS_PARAMS, 3},
      {"radii-flower", RADII_FLOWER_PARAMS, 3},
      {"radii-lotus", RADII_LOTUS_PARAMS, 3},
      {"radii-radial", RADII_RADIAL_PARAMS, 3},
      {"radii-lollipop", RADII_LOLLIPOP_PARAMS, 5},
      {"animartrix-polarwaves", ANIMARTRIX_POLARWAVES_PARAMS, 10},
      {"animartrix-spiralus", ANIMARTRIX_SPIRALUS_PARAMS, 9},
      {"animartrix-caleido1", ANIMARTRIX_CALEIDO1_PARAMS, 9},
      {"animartrix-coolwaves", ANIMARTRIX_COOLWAVES_PARAMS, 9},
      {"animartrix-chasingspirals", ANIMARTRIX_CHASINGSPIRALS_PARAMS, 11},
      {"animartrix-complexkaleido6", ANIMARTRIX_COMPLEXKALEIDO6_PARAMS, 12},
      {"animartrix-water", ANIMARTRIX_WATER_PARAMS, 7},
      {"animartrix-experiment1", ANIMARTRIX_EXPERIMENT1_PARAMS, 7},
      {"animartrix-experiment2", ANIMARTRIX_EXPERIMENT2_PARAMS, 9},
      {"animartrix-fluffyblobs", ANIMARTRIX_FLUFFYBLOBS_PARAMS, 10},
      {"animartrix-test3", ANIMARTRIX_TEST3_PARAMS, 9},
      {"synaptide", SYNAPTIDE_PARAMS, 16},
      {"cube", CUBE_PARAMS, 4},
      {"horizons", HORIZONS_PARAMS, 3},
      {"audiotest-spectrumbars", AUDIOTEST_SPECTRUMBARS_PARAMS, 0},
      {"audiotest-vumeter", AUDIOTEST_VUMETER_PARAMS, 0},
      {"audiotest-beatpulse", AUDIOTEST_BEATPULSE_PARAMS, 0},
      {"audiotest-bassripple", AUDIOTEST_BASSRIPPLE_PARAMS, 0},
      {"audiotest-flbeatdetection", AUDIOTEST_LATENCYTEST_PARAMS, 0},
      {"audiotest-radialspectrum", AUDIOTEST_RADIALSPECTRUM_PARAMS, 0},
      {"audiotest-waveform", AUDIOTEST_WAVEFORM_PARAMS, 0},
      {"audiotest-spectrogram", AUDIOTEST_SPECTROGRAM_PARAMS, 0},
      {"audiotest-finespectrum", AUDIOTEST_FINESPECTRUM_PARAMS, 0},
      {"audiotest-busbeats", AUDIOTEST_BUSBEATS_PARAMS, 0},
   };

  class VisualizerManager {
  public:
      static String getVisualizerName(int programNum, int mode = -1) {
          if (programNum < 0 || programNum > PROGRAM_COUNT-1) return "";

          // Get program name from flash memory
          char progName[16];
          ::strcpy(progName,(char*)pgm_read_ptr(&PROGRAM_NAMES[programNum]));

          if (mode < 0 || MODE_COUNTS[programNum] == 0) {
              return String(progName);
          }

          // Get mode name
          const char* const* modeArray = nullptr;
          switch (programNum) {
              case WAVES: modeArray = WAVES_MODES; break;
              case RADII: modeArray = RADII_MODES; break;
              case ANIMARTRIX: modeArray = ANIMARTRIX_MODES; break;
              case AUDIOTEST: modeArray = AUDIOTEST_MODES; break;
              default: return String(progName);
          }

          if (mode >= MODE_COUNTS[programNum]) return String(progName);

          char modeName[20];
          ::strcpy(modeName,(char*)pgm_read_ptr(&modeArray[mode]));

         String result = "";
         result += String(progName);
         result += "-";
         result += String(modeName);
         return result;
      }
      
      // Get parameter list based on visualizer name
      static const VisualizerParamEntry* getVisualizerParams(const String& visualizerName) {
          const int LOOKUP_SIZE = sizeof(VISUALIZER_PARAM_LOOKUP) / sizeof(VisualizerParamEntry);
          
          for (int i = 0; i < LOOKUP_SIZE; i++) {
              char entryName[32];
              ::strcpy(entryName, (char*)pgm_read_ptr(&VISUALIZER_PARAM_LOOKUP[i].visualizerName));
              
              if (visualizerName.equals(entryName)) {
                  return &VISUALIZER_PARAM_LOOKUP[i];
              }
          }
          return nullptr;
      }
  };  // class VisualizerManager


  // AUDIO SETTINGS ==================================================

    typedef void (*BusParamSetterFn)(uint8_t busId, const String& paramId, float value);
    BusParamSetterFn setBusParam = nullptr;

    // Callback to read a bus parameter value by busId and param name
    typedef float (*BusParamGetterFn)(uint8_t busId, const String& paramName);
    BusParamGetterFn getBusParam = nullptr;


   const char* const AUDIO_PARAMS[] PROGMEM = {
      "maxBins", "audioFloor", "audioGain",
      "avLevelerTarget", "autoFloorAlpha", "autoFloorMin", "autoFloorMax",
      "noiseGateOpen", "noiseGateClose",
      "threshold", "minBeatInterval",
      "rampAttack", "rampDecay", "peakBase", "expDecayFactor"
    };

   const uint8_t AUDIO_PARAM_COUNT = 15;


   // for animartrix CK_6 ===================================
   struct StarParams {
      float starAngle = 3.f; 
      float starScale = 1.f;
      float starZoom = 1.f;
      float starTwist = 1.f;
      float starZ = 1.f;
   };
   StarParams starParams[10]; 

   
// ═══════════════════════════════════════════════════════════════════
//  MISCELLANEOUS CONTROLS
// ═══════════════════════════════════════════════════════════════════

uint8_t cBright = 35;
uint8_t cMapping = 0;
uint8_t cOverrideMapping = 0;

fl::EaseType getEaseType(uint8_t value) {
    switch (value) {
        case 0: return fl::EaseType::EASE_NONE;
        case 1: return fl::EaseType::EASE_IN_QUAD;
        case 2: return fl::EaseType::EASE_OUT_QUAD;
        case 3: return fl::EaseType::EASE_IN_OUT_QUAD;
        case 4: return fl::EaseType::EASE_IN_CUBIC;
        case 5: return fl::EaseType::EASE_OUT_CUBIC;
        case 6: return fl::EaseType::EASE_IN_OUT_CUBIC;
        case 7: return fl::EaseType::EASE_IN_SINE;
        case 8: return fl::EaseType::EASE_OUT_SINE;
        case 9: return fl::EaseType::EASE_IN_OUT_SINE;
    }
    FL_ASSERT(false, "Invalid ease type");
    return fl::EaseType::EASE_NONE;
}

uint8_t cEaseSat = 0;
uint8_t cEaseLum = 0;


// ═══════════════════════════════════════════════════════════════════
//  PARAMETER DECLARATIONS
// ═══════════════════════════════════════════════════════════════════

// Waves
bool rotateWaves = true; 
float cHueIncMax = 2500;
uint8_t cBlendFract = 128;
float cBrightTheta = 1;

// animARTrix/common
float cSpeed = 1.f;
float cZoom = 1.f;
float cScale = 1.f;
float cAngle = 1.f;
float cTwist = 1.f;
float cRadius = 1.0f;
float cRadialSpeed = 1.0f;
uint8_t cLinearSpeed = 5;
float cEdge = 1.0f;
float cZ = 1.f; 
uint8_t cSpeedInt = 1;
uint8_t cStarParamSet = 0;

bool Layer1 = true;
bool Layer2 = true;
bool Layer3 = true;
bool Layer4 = true;
bool Layer5 = true;
bool Layer6 = true;
bool Layer7 = true;
bool Layer8 = true;
bool Layer9 = true;

// animARTrix
float cRatBase = 0.0f; 
float cRatDiff= 1.f; 
float cOffBase = 1.f; 
float cOffDiff = 1.f; 
uint8_t cFxIndex = 0;
uint8_t cColOrd = 0;

float cRed = 1.f; 
float cGreen = 1.f; 
float cBlue = 1.f;

//fxWave2d
float cSpeedLower = .16f;
float cDampLower = 8.f;
float cSpeedUpper = .24f;
float cDampUpper = 8.f;
float cBlurGlobFact = 1.f;
bool synaptideResetRequested = false;   //bool fancyTrigger = false;

//Bubble
float cMovement = 1.f;

//Dots
float cTail = 1.f;

//Synaptide
float cSynSpeed = 1.0f;  // global animation speed (fractional-step lerp of next state)
float cBloomEdge = 1.0f;
// Changed from double -> float: P4 FPU is single-precision only; double
// arithmetic goes through software emulation (~200-500 cycles per op vs
// ~3-5 for hardware float). These params appear in hot per-pixel loops.
float cDecayBase = 0.95f;       //double cDecayBase = .95;
float cDecayChaos = 0.05f;      //double cDecayChaos = .05;
float cIgnitionBase = 0.16f;    //double cIgnitionBase = .16;
float cIgnitionChaos = 0.05f;   //double cIgnitionChaos = .05;
float cNeighborBase = 0.48f;    //double cNeighborBase = .48;
float cNeighborChaos = 0.08f;   //double cNeighborChaos = .08;
float cSpatialDecay = 0.002f;
float cDecayZones = 1.0f;
float cTimeDrift = 1.0f;
float cPulse = 1.0f;
float cInfluenceBase = 0.7f;    //double cInfluenceBase = 0.7;
float cInfluenceChaos = 0.35f;  //double cInfluenceChaos = 0.35;
uint16_t cEntropyRate = 120;
float cEntropyBase = 0.05f;
float cEntropyChaos = 0.15f;

//Cube
float cAngleRateX = 0.02f;
float cAngleRateY = 0.03f;
float cAngleRateZ = 0.01f;
bool cAngleFreezeX = false;
bool cAngleFreezeY = false;
bool cAngleFreezeZ = false;

//Horizons
bool cycleDurationManualMode = false; 
bool restartTriggered = false;
bool rotateUpperTriggered = false;
bool rotateLowerTriggered = false;
uint8_t cLightBias = 0;
uint8_t cDramaScale = 0;
uint8_t cCycleDuration = 2;
bool sceneManualMode = false;
bool updateScene = false;
bool updateCycleTiming = false;


// AUDIO -----------------------
bool maxBins = false;
uint16_t cNoiseGateOpen = 70;
uint16_t cNoiseGateClose = 50;
float cAudioGain = 1.0f;
float cAudioFloor = 0.0f;
bool autoFloor = false;
float cAutoFloorAlpha = 0.01f;
float cAutoFloorMin = 0.0f;
float cAutoFloorMax = 0.5f;
bool avLeveler = true;
float cAvLevelerTarget = 0.5f;
float cThreshold = 0.40f;
float cMinBeatInterval = 75.f;
float cRampAttack = 0.f;
float cRampDecay = 100.f;
float cPeakBase = 1.0f;
float cExpDecayFactor = 0.9f;

// ═══════════════════════════════════════════════════════════════════
//  X-MACRO PARAMETER TABLE
// ═══════════════════════════════════════════════════════════════════

// X-Macro table 
#define PARAMETER_TABLE \
   X(uint8_t, OverrideMapping, 0) \
   X(uint8_t, ColOrd, 1.0f) \
   X(float, Speed, 1.0f) \
   X(float, Zoom, 1.0f) \
   X(float, Scale, 1.0f) \
   X(float, Angle, 1.0f) \
   X(float, Twist, 1.0f) \
   X(uint8_t, LinearSpeed, 5) \
   X(float, RadialSpeed, 1) \
   X(float, Radius, 1.0f) \
   X(float, Edge, 1.0f) \
   X(float, Z, 1.0f) \
   X(float, RatBase, 0.0f) \
   X(float, RatDiff, 1.0f) \
   X(float, OffBase, 1.0f) \
   X(float, OffDiff, 1.0f) \
   X(float, Red, 1.0f) \
   X(float, Green, 0.8f) \
   X(float, Blue, 0.6f) \
   X(uint8_t, SpeedInt, 1) \
   X(uint8_t, StarParamSet, 0) \
   X(float, HueIncMax, 2500.0f) \
   X(uint8_t, BlendFract, 128) \
   X(float, BrightTheta, 1.0f) \
   X(float, SpeedLower, .16f) \
   X(float, DampLower, 8.0f) \
   X(float, SpeedUpper, .24f) \
   X(float, DampUpper, 6.0f) \
   X(float, BlurGlobFact, 1.0f) \
   X(float, Movement, 1.0f) \
   X(float, Tail, 1.0f) \
   X(uint8_t, EaseSat, 0) \
   X(uint8_t, EaseLum, 0) \
   X(float, SynSpeed, 1.0f) \
   X(float, BloomEdge, 1.0f) \
   X(float, DecayBase, 0.95f) \
   X(float, DecayChaos, 0.04f) \
   X(float, IgnitionBase, 0.16f) \
   X(float, IgnitionChaos, 0.05f) \
   X(float, NeighborBase, 0.48f) \
   X(float, NeighborChaos, 0.06f) \
   X(float, SpatialDecay, 0.002f) \
   X(float, DecayZones, 1.0f) \
   X(float, TimeDrift, 1.0f) \
   X(float, Pulse, 1.0f) \
   X(float, InfluenceBase, 0.7f) \
   X(float, InfluenceChaos, 0.35f) \
   X(uint16_t, EntropyRate, 180) \
   X(float, EntropyBase, 0.05f) \
   X(float, EntropyChaos, 0.15f) \
   X(float, AngleRateX, 0.02f) \
   X(float, AngleRateY, 0.03f) \
   X(float, AngleRateZ, 0.01f) \
   X(bool, AngleFreezeX, false) \
   X(bool, AngleFreezeY, false) \
   X(bool, AngleFreezeZ, false) \
   X(uint8_t, LightBias, 5) \
   X(uint8_t, DramaScale, 5) \
   X(uint8_t, CycleDuration, 2) \
   X(float, AudioGain, 1.0f) \
   X(float, AvLevelerTarget, 0.5f) \
   X(float, AudioFloor, 0.05f) \
   X(float, AutoFloorAlpha, 0.05f) \
   X(float, AutoFloorMin, 0.0f) \
   X(float, AutoFloorMax, 0.05f) \
   X(uint16_t, NoiseGateOpen, 70) \
   X(uint16_t, NoiseGateClose, 50) \
   X(float, Threshold, 0.0f) \
   X(float, MinBeatInterval, 0.0f) \
   X(float, RampAttack, 0.f) \
   X(float, RampDecay, 150.f) \
   X(float, PeakBase, 1.0f) \
   X(float, ExpDecayFactor, 1.0f)
