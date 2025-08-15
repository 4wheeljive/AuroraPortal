# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

AuroraPortal is a FastLED-based LED matrix controller project for ESP32 microcontrollers. It features multiple LED animation patterns, Bluetooth Low Energy (BLE) control, and a web-based control interface. The project creates dynamic LED displays with various visual effects and real-time parameter control.

## Development Commands

### Build and Flash
- `pio run` - Build the project
- `pio run -t upload` - Build and upload to ESP32
- `pio run -t uploadfs` - Upload filesystem (for LittleFS files)
- `pio run -t monitor` - Open serial monitor
- `pio run -t clean` - Clean build files

### Development Environment
- Uses PlatformIO framework with Arduino core for ESP32
- Target board: Seeed XIAO ESP32S3
- Debug build configuration with ESP32 exception decoder
- LittleFS filesystem for preset storage

## Architecture Overview

### Hardware Configuration
- **LED Matrix Support**: Two configurations available
  - Small board: 24x24 matrix (576 LEDs, single data pin)
  - Big board: 32x48 matrix (1536 LEDs, 3 data pins for parallel output)
- **Board Selection**: Controlled by `#define BIG_BOARD` in main.cpp:51
- **Data Pins**: PIN_1 (GPIO2), PIN_2 (GPIO3), PIN_3 (GPIO4) for big board
- **Matrix Mapping**: Custom XY coordinate mapping via lookup tables in matrixMap_*.h files

### Core Components

#### LED Pattern System (src/main.cpp)
- **Program-based architecture**: 7 main programs (RAINBOW, WAVES, BUBBLE, DOTS, fxWAVE2d, RADII, ANIMARTRIX)
- **Mode system**: Each program has multiple sub-modes for pattern variations
- **Real-time parameter control**: Speed, brightness, and pattern-specific parameters
- **FastLED integration**: Uses FastLED library with WS2812B LED strips

#### BLE Control System (src/bleControl.h)
- **Three-characteristic BLE interface**: Button, Checkbox, and Number characteristics
- **JSON-based communication**: Structured data exchange for parameters
- **Preset system**: Save/load up to 10 parameter presets to LittleFS
- **Real-time feedback**: Bidirectional communication with confirmation messages

#### Web Interface (index.html)
- **Custom web components**: Modular UI built with vanilla JavaScript
- **BLE connectivity**: Direct browser-to-ESP32 communication via Web Bluetooth API
- **Parameter controls**: Real-time sliders, buttons, and checkboxes
- **Responsive design**: Grid-based layout optimized for mobile devices

#### Animation Engines
- **FastLED Animartrix**: Advanced 2D effects using FastLED's fx engine (src/myAnimartrix.hpp)
- **Custom patterns**: Pride waves, rainbow matrix, dot dance, radial effects
- **Wave effects**: 2D wave physics simulation with dual-layer blending
- **Soap bubble**: Noise-based fluid simulation effects

### File Structure

**Framework Core:**
- **src/main.cpp**: Main program loop, hardware setup, program switching, global mappings
- **src/bleControl.h**: BLE server, characteristic handling, preset management, cVariable parameters
- **src/matrixMap_24x24.h**: LED coordinate mapping for small board (576 LEDs, single pin)  
- **src/matrixMap_32x48_3pin.h**: LED coordinate mapping for big board (1536 LEDs, 3 pins)
- **platformio.ini**: Build configuration, dependencies, board settings
- **index.html**: Complete web-based control interface with Web Bluetooth API

**LED Program Modules:** *(Each program has .hpp interface + _detail.hpp implementation)*
- **src/rainbow.hpp** + **src/rainbow_detail.hpp**: Simple rainbow matrix animation  
- **src/waves.hpp** + **src/waves_detail.hpp**: Pride wave algorithm with palette rotation
- **src/bubble.hpp** + **src/bubble_detail.hpp**: Noise-based fluid simulation effects
- **src/dots.hpp** + **src/dots_detail.hpp**: Oscillator-based particle system with trails
- **src/fxWaves2d.hpp** + **src/fxWaves2d_detail.hpp**: Complex FastLED fx engine wave physics
- **src/radii.hpp** + **src/radii_detail.hpp**: Polar coordinate mathematics (4 modes)
- **src/myAnimartrix.hpp** + **src/myAnimartrix_detail.hpp**: FastLED Animartrix integration

**Program Organization Notes:**
- Each program uses the circular dependency pattern: main.cpp → program.hpp → program_detail.hpp → bleControl.h
- Detail files contain all implementation and can access cVariable parameters for future real-time control
- Programs with XY coordinate mapping use function pointer pattern for main.cpp XY function access
- Complex programs (fxWaves2d) use dynamic object creation with XYMap parameter passing

### Key Integration Points
- **XY Mapping**: Custom coordinate system connects logical XY to physical LED indices
- **Parameter System**: Unified parameter handling across BLE, web interface, and pattern engines
- **Animation State**: Global variables in bleControl.h manage pattern parameters and state
- **Preset Storage**: LittleFS-based persistence for user configurations

### Development Notes
- **Board configuration**: Always verify BIG_BOARD define matches your hardware
- **LED mapping**: Matrix coordinate files must match your physical LED strip arrangement
- **BLE limitations**: ESP32 BLE has handle limits; see bleControl.h:13-16 for configuration
- **Web Bluetooth**: Requires HTTPS or localhost for browser security
- **Serial debugging**: Enable via `debug = true` in bleControl.h:29

## Current Refactoring Status

### Modular Architecture Migration (In Progress)

**Goal**: Extract LED pattern programs from main.cpp into modular architecture to enable future real-time parameter control. Each program gets two files following the circular dependency pattern established by Animartrix.

**Pattern Used**:
1. `programName.hpp` - Interface file with namespace and function declarations
2. `programName_detail.hpp` - Implementation file that includes bleControl.h and contains all program code
3. main.cpp modification - Add include, update switch case to use namespace::initProgram() and namespace::runProgram()
4. Original code commented out in main.cpp for reference

**Critical Technical Detail - XY Function Access**:
Programs that use LED coordinate mapping via `XY(x, y)` function encounter compilation errors because the XY function is defined in main.cpp but detail files only include bleControl.h. Solution pattern:

1. Add function pointer in detail file: `uint16_t (*xyFunc)(uint8_t x, uint8_t y);`
2. Modify initProgram() to accept XY function: `void initProgram(uint16_t (*xy_func)(uint8_t, uint8_t))`
3. Store function pointer: `xyFunc = xy_func;`
4. Replace all `XY(x, y)` calls with `xyFunc(x, y)` in program code
5. Update main.cpp call to pass XY function: `initProgram(XY);`

This maintains the circular dependency structure while allowing coordinate mapping access.

**Advanced Pattern - Complex Object Access**:
For programs using complex FastLED objects (WaveFx, Blend2d) that need XYMap constructor parameters:

1. Add object pointers in detail file: `WaveFx* waveFxLowerPtr = nullptr;`
2. Modify initProgram() to accept XYMap objects: `void initProgram(XYMap& myXYmap, XYMap& xyRect)`
3. Store object references: `myXYmapPtr = &myXYmap;`
4. Create objects dynamically: `waveFxLowerPtr = new WaveFx(myXYmap, CreateArgsLower());`
5. Replace all object calls with pointer access: `waveFxLowerPtr->method()`
6. Update main.cpp call to pass objects: `initProgram(myXYmap, xyRect);`

**Extraction Status**:
- ✅ **ANIMARTRIX** - Already implemented (original pattern)
- ✅ **BUBBLE** - Already implemented (src/bubble.hpp, src/bubble_detail.hpp)
- ✅ **RADII** - **COMPLETED & TESTED** (src/radii.hpp, src/radii_detail.hpp)
  - Original code: main.cpp lines 888-937 (now commented out at lines 877-942)
  - Namespace: `radii::`
  - Instance tracking: `radiiInstance`
  - All globals moved: `setupm`, `rMap`, `legs`, `C_X`, `C_Y`, `mapp`
- ✅ **DOTS** - **COMPLETED & TESTED** (src/dots.hpp, src/dots_detail.hpp)
  - Original code: main.cpp lines 538-597 (now commented out at lines 537-602)
  - Namespace: `dots::`
  - Instance tracking: `dotsInstance`
  - All globals moved: `osci[4]`, `pX[4]`, `pY[4]`
  - All functions moved: `PixelA()`, `PixelB()`, `MoveOscillators()`, `VerticalStream()`
  - Uses XY function pointer pattern
- ✅ **fxWAVE2D** - **COMPLETED & TESTED** (src/fxWaves2d.hpp, src/fxWaves2d_detail.hpp)
  - Original code: main.cpp lines 607-879 (now commented out at lines 607-884)
  - Namespace: `fxWaves2d::`
  - Instance tracking: `fxWaves2dInstance`
  - Complex FastLED fx engine integration preserved as-is
  - All variables moved: `firstWave`, wave configs, palettes, fx objects
  - All functions moved: `CreateArgsLower/Upper()`, `getSuperSample()`, etc.
  - Existing cVariable integration maintained (`cRatDiff`, `cOffBase`, `cOffDiff`, `cZ`)
  - Uses EVERY_N_MILLISECONDS_RANDOM macro
- ✅ **WAVES** - **COMPLETED & TESTED** (src/waves.hpp, src/waves_detail.hpp)
  - Original code: main.cpp lines 308-408 (now commented out at lines 307-413)
  - Namespace: `waves::`
  - Instance tracking: `wavesInstance`
  - Pride wave algorithm with palette rotation
  - Uses EVERY_N_SECONDS and EVERY_N_MILLISECONDS macros
  - Static variables: `sPseudotime`, `sLastMillis`, `sHue16`
  - Uses existing global arrays: `loc2indProg[]`, `loc2indSerp[]`
  - No XY function dependency 
- ✅ **RAINBOW** - **COMPLETED & TESTED** (src/rainbow.hpp, src/rainbow_detail.hpp)
  - Original code: main.cpp lines 276-303 (now commented out at lines 277-308)
  - Namespace: `rainbow::`
  - Instance tracking: `rainbowInstance`
  - Simple rainbow matrix with cos16-based animation
  - Two functions: `DrawOneFrame()` helper and `runRainbow()` main
  - Uses existing global arrays: `loc2indProgByRow[]`, `loc2indSerpByRow[]`
  - No static variables or complex dependencies

**Important Notes for Continuation**:
- Preserve ALL existing global variables, EVERY_N_* macros, and timing exactly as-is
- Do NOT add new parameter integrations - this is purely architectural separation
- Maintain FastLED fx engine integrations without modification
- Test each extraction before proceeding to the next
- Original code is commented out in main.cpp for troubleshooting reference

## 🎉 MODULAR ARCHITECTURE MIGRATION COMPLETE! 🎉

**ALL 7 LED PROGRAMS SUCCESSFULLY EXTRACTED AND TESTED:**
- Each program now lives in its own modular namespace
- All existing functionality preserved exactly as-is
- Infrastructure in place for future real-time parameter control
- Clean separation enables individual program development
- Circular dependency pattern established and documented

**Mission Accomplished!** The codebase is now ready for the next phase of development.