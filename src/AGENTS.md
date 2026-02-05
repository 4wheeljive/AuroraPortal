# AGENTS.md

IMPORTANT: Take the information below with a grain of salt...it often ends up with inaccurate or outdated information. 

## Project Overview

AuroraPortal is a FastLED-based LED matrix controller project for ESP32 microcontrollers. It features multiple LED animation patterns, Bluetooth Low Energy (BLE) control, and a web-based control interface. The project creates dynamic LED displays with various visual effects and real-time parameter control. The current development focus is adding audio input into visual effects.

## Development Environment
- Uses pioarduino implementation of PlatformIO framework with Arduino core for ESP32
- Target board: Seeed XIAO ESP32S3
- Debug build configuration with ESP32 exception decoder
- LittleFS filesystem for preset storage

## Architecture Overview

### Hardware Configuration
- **LED Matrix Support**: Two configurations available
  - Small board: 22x22 matrix (484 LEDs, single data pin)
  - Big board: 32x48 matrix (1536 LEDs, 3 data pins for parallel output)
- **Board Selection**: Controlled by `#define BIG_BOARD` in main.cpp
- **Data Pins**: PIN_0 (GPIO2), PIN_1 (GPIO3), PIN_2 (GPIO4) for big board
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
- **src/audioInput.h**: audio engine
- **src/audioProcessing.h**: audio engine
- **src/matrixMap_22x22.h**: LED coordinate mapping for small board (484 LEDs, single pin)  
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
- **Ignored files**: Do not modify or reference files in the `/ignore` folder - these are experimental/deprecated files
- **Visualizer Concept**: A "visualizer" refers to either (a) a specific program-mode combination (e.g., "radii-octopus") or (b) a modeless program (e.g., "dots").

### Parameter Management System**:
- Standardized naming convention for parameter variables: `cParameter` (current), `inParameter` (UI input/display)
- **JSON-based preset containers** storing multiple parameters + visualizer name
- **LittleFS file persistence** (e.g., `/preset_1.json`, `/preset_2.json`) 
- **X-macros for code generation only** - automating repetitive declarations and functions
- **Visualizer name identification** instead of index numbers for future-proofing

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

### BLE Connection State Management (In progress)
**Problem**: Web components loaded without knowing device connection status, causing confusion when BLE was disconnected.
