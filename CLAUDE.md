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
- **Mode system**: Several programs have multiple sub-modes for pattern variations
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
- **FastLED Animartrix**: Advanced 2D effects using FastLED's fx engine
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
- **src/programs/rainbow.hpp** + **src/programs/rainbow_detail.hpp**: Simple rainbow matrix animation  
- **src/programs/waves.hpp** + **src/programs/waves_detail.hpp**: Pride wave algorithm with palette rotation
- **src/programs/bubble.hpp** + **src/programs/bubble_detail.hpp**: Noise-based fluid simulation effects
- **src/programs/dots.hpp** + **src/programs/dots_detail.hpp**: Oscillator-based particle system with trails
- **src/programs/fxWaves2d.hpp** + **src/programs/fxWaves2d_detail.hpp**: Complex FastLED fx engine wave physics
- **src/programs/radii.hpp** + **src/programs/radii_detail.hpp**: Polar coordinate mathematics (4 modes)
- **src/programs/animartrix.hpp** + **src/programs/animartrix_detail.hpp**: FastLED Animartrix integration

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
- **Ignored files**: Do not modify or reference files in the `/ignore` folder - these are experimental/deprecated files

## Planned Enhancements

### Visualizer Concept (In Development)
**Definition**: A "visualizer" refers to either (a) a specific program/mode combination (e.g., "radii::octopus") or (b) a modeless program (e.g., "dots"). Each visualizer has a unique name/ID.

**Purpose**: Enable dynamic parameter control based on the currently active visualization.

### Dynamic Parameter System (Planned)
**Architecture**:
- **ControlSlider**: Rename current ParameterSlider class for standalone slider deployment
- **Parameter Registry**: Centralized parameter definitions with standard ranges (min/max/default/label)
- **ParameterSettings**: Auto-rendering class (like ProgramSelector/ModeSelector) that dynamically shows sliders for current visualizer

**Key Design Decisions**:
- Parameters use standard ranges; visualization code adapts as needed
- State synchronization via updateParameters() method called from selectProgram()/selectMode()
- Parameters reset to defaults when switching visualizers
- ParameterSettings auto-hides unused parameters
- Parameter persistence handled at ESP32 level (preferences.begin/getUChar pattern)

