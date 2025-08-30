# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

IMPORTANT: Take the information below with a grain of salt...it often ends up with inaccurate or outdated information. 

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
- **Program-based architecture**: Several main programs (e.g., RAINBOW, WAVES, BUBBLE, DOTS, fxWAVE2d, RADII, ANIMARTRIX,...)
- **Mode system**: Several programs have multiple sub-modes for pattern variations
- **Real-time parameter control**: Speed, brightness, color, and pattern-specific parameters
- **FastLED integration**: Uses FastLED library with WS2812B LED strips

#### BLE Control System (src/bleControl.h)
- **Four-characteristic BLE interface**: Button, Checkbox, Number and String characteristics
- **JSON-based communication**: Structured data exchange for parameters
- **Preset system**: Save/load parameter presets to LittleFS
- **Real-time feedback**: Bidirectional communication with confirmation messages

#### Web Interface (index.html)
- **Custom web components**: Modular UI built with vanilla JavaScript
- **BLE connectivity**: Direct browser-to-ESP32 communication via Web Bluetooth API
- **Parameter controls**: Real-time sliders, buttons, checkboxes, dropdown menus, etc.
- **Responsive design**: Grid-based layout optimized for mobile devices

#### Animation Engines
- **Stand-alone patterns**: Pride waves, rainbow matrix, dot dance, radial effects
- **Wave effects**: 2D wave physics simulation with dual-layer blending
- **FastLED Animartrix** animations/modes: Advanced 2D effects using FastLED's fx engine

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
- **src/programs/fxWave2d.hpp** + **src/programs/fxWave2d_detail.hpp**: Complex FastLED fx engine wave physics
- **src/programs/radii.hpp** + **src/programs/radii_detail.hpp**: Polar coordinate mathematics (4 modes)
- **src/programs/animartrix.hpp** + **src/programs/animartrix_detail.hpp**: FastLED Animartrix integration
- other programs added periodically

**Program Organization Notes:**
- Each program uses the circular dependency pattern: main.cpp → program.hpp → program_detail.hpp → bleControl.h
- Detail files contain all implementation and can access cVariable parameters for future real-time control
- Programs with XY coordinate mapping use function pointer pattern for main.cpp XY function access
- Complex programs (fxWave2d) use dynamic object creation with XYMap parameter passing

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

## Historical Context and Background

### AuroraPortal + Animartrix Integration Background
**Original Architecture Separation**:
- **Original AuroraPortal**: Program/Mode system, but no runtime parameter control
- **Original Animartrix**: Runtime parameter control and preset system, but no program/mode concept  

**Current Hybrid State**: Animartrix became "program 6" with its cFxIndex functioning as modes within AuroraPortal's broader architecture

**Integration Challenges**:
- Phasing out old bleControl.h code that came from Animartrix
- Need to refactor several remaining functions to work within new/broader AuroraPortal context

### Parameter Management Evolution
**Original Animartrix Parameter System**:
- Used struct-based Preset architecture with mixed data types (string, uint8_t, float)
- Required manual code lines for every parameter in every function (`updateUI()`, `resetAll()`, `processNumber()`, etc.)
- Worked adequately with ~15 parameters but became increasingly cumbersome
- Used standardized naming: `cParameter` (current), `pParameter` (preset storage), `inParameter` (UI input/display)

**Problem with Struct Approach**:
- Manual entry required for every parameter in every function
- Error-prone when adding parameters (easy to forget updating functions)  
- Not scalable as more AuroraPortal programs need additional parameters
- No automation possible due to heterogeneous data types

**X-Macro Solution Concept**:
- Identified that standardized naming convention (cZoom, pZoom, inZoom) could enable automation
- X-macros could auto-generate repetitive getter/setter/processing functions
- Single source of truth in parameter table
- New parameters can be implementd by adding dcode in just several places 

**Key Changes**: Using JSON as the preset container (replacing struct) while using X-macros to automate the repetitive code that handles those JSON presets.

## Recent Enhancements

### Visualizer Concept (Implemented)
**Definition**: A "visualizer" refers to either (a) a specific program-mode combination (e.g., "radii-octopus") or (b) a modeless program (e.g., "dots").

**Implementation**: JavaScript-based system in index.html that maps applicable parameters to each visualizer

### Dynamic Parameter System (Implemented)
**Architecture**:
- **ControlSlider**: Standalone slider component for manual deployment
- **Parameter Registry**: Centralized definitions for all parameters (speed, zoom, scale, angle, radius, edge, z)
- **ParameterSettings**: Auto-rendering component that dynamically shows sliders based on current visualizer
- **Helper Functions**: Auto-generate parameter labels and IDs from parameter names

**Key Features**:
- **Dynamic Rendering**: ParameterSettings automatically shows/hides relevant sliders when visualizer changes
- **State Synchronization**: Updates triggered by selectProgram()/selectMode() methods
- **Parameter Reset**: All parameters reset to defaults when switching visualizers
- **Consistent Styling**: Uses existing slider styling and BLE communication patterns
- **Dual Deployment**: Supports both standalone ControlSlider instances and dynamic ParameterSettings

## Recent Updates

### BLE Connection State Management (In progress)
**Problem**: Web components loaded without knowing device connection status, causing confusion when BLE was disconnected.

**Solution**: Global BLE state manager with component coordination:
- **BLE State Manager**: `window.BLEState` tracks connection status and current device state
- **Loading States**: Components show "Connect device for [options]" when disconnected
- **Connection Handlers**: Auto-sync device state on connection, reset on disconnection
- **Component Updates**: All components automatically update when connection state changes
- **Initial State Sync**: `syncInitialState()` reads current component state and syncs with device

### Parameter Update System (Fixed)
**Problem**: `applyReceivedNumber()` couldn't update ParameterSettings sliders when presets were loaded.

**Solution**: Debounced batch re-rendering approach:
- **Standalone Sliders**: Update immediately via `controlSlider.updateValue()`
- **ParameterSettings**: Batch multiple parameter updates and re-render component after 150ms delay
- **Efficient**: Handles rapid parameter changes (preset loading) with single re-render
- **Reliable**: Always shows correct values by re-rendering from current state

### NEW PARAMETER/PRESET MANAGEMENT SYSTEM "PPMS"

#### Core Architecture
- **JSON-based preset containers** storing multiple parameters + visualizer name
- **LittleFS file persistence** (e.g., `/preset_1.json`, `/preset_2.json`) 
- **X-macros for code generation only** - automating repetitive declarations and functions
- **Visualizer name identification** instead of index numbers for future-proofing


### Device State Sync on BLE Connection (NEXT PRIORITY)
Largely complete. Some bug squashing required
