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

## Recent Enhancements

### Visualizer System (Implemented)
**Definition**: A "visualizer" refers to either (a) a specific program/mode combination (e.g., "radii::octopus") or (b) a modeless program (e.g., "dots"). Each visualizer has a unique name/ID.

**Implementation**: JavaScript-based system in index.html that maps all visualizers to their required parameters.

### Dynamic Parameter System (Implemented)
**Architecture**:
- **ControlSlider**: Standalone slider component (renamed from ParameterSlider) for manual deployment
- **Parameter Registry**: Centralized definitions for 7 standard parameters (speed, zoom, scale, angle, radius, edge, z)
- **ParameterSettings**: Auto-rendering component that dynamically shows sliders based on current visualizer
- **Helper Functions**: Auto-generate parameter labels and IDs from parameter names

**Key Features**:
- **Dynamic Rendering**: ParameterSettings automatically shows/hides relevant sliders when visualizer changes
- **State Synchronization**: Updates triggered by selectProgram()/selectMode() methods
- **Parameter Reset**: All parameters reset to defaults when switching visualizers
- **Consistent Styling**: Uses existing slider styling and BLE communication patterns
- **Dual Deployment**: Supports both standalone ControlSlider instances and dynamic ParameterSettings

## Recent Updates

### BLE Connection State Management (Implemented)
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

### X-Macro Parameter System (Experimental)
**Goal**: Simplify parameter management to avoid manual code updates for each new parameter.

**Current Challenges with Struct-Based Presets**:
- Manual code required in every function (`updateUI()`, `resetAll()`, `processNumber()`, etc.) for each parameter
- Increasingly cumbersome as more parameters are added for different AuroraPortal programs
- Error-prone when adding parameters (easy to forget updating a function)

**X-Macro Solution Implemented** (Test Phase):
- **Single Source**: `CUSTOM_PARAMETER_TABLE` defines parameters once using X-macro pattern
- **Auto-Generated Functions**: Getter/setter functions automatically handle all parameters
- **JSON-Based Presets**: Replace manual struct file I/O with JSON serialization
- **Type-Safe**: Compile-time type checking with support for multiple data types

**Test Implementation** (CustomA-E parameters only):
```cpp
#define CUSTOM_PARAMETER_TABLE \
    X(float, CustomA, 1.0f) \
    X(float, CustomB, 1.0f) \
    X(float, CustomC, 1.0f) \
    X(float, CustomD, 1.0f) \
    X(uint8_t, CustomE, 1)
```

**Available Test Functions**:
- `captureCustomPreset(JsonDocument& preset)` - Auto-captures all custom parameters
- `applyCustomPreset(const JsonDocument& preset)` - Auto-applies all custom parameters  
- `setCustomFloatValue(String id, float value)` - Auto-generated parameter setter
- `getCustomFloatValue(String id)` - Auto-generated parameter getter

**Next Steps for X-Macro Rollout**:
1. **Test Phase**: Validate X-macro approach with CustomA-E parameters
   - Test compilation and runtime behavior
   - Verify JSON preset capture/apply functionality
   - Ensure BLE communication works correctly
2. **Expand Gradually**: Add more parameter types to CUSTOM_PARAMETER_TABLE
3. **Integration**: Modify existing functions to use X-macro helpers
4. **Migration**: Replace manual parameter handling with auto-generated functions
5. **Full Replacement**: Eventually replace entire struct-based preset system

**Benefits Once Fully Implemented**:
- **One-Line Parameter Addition**: Just add line to PARAMETER_TABLE
- **Automatic Function Updates**: All getter/setter/preset functions update automatically  
- **No Manual Maintenance**: Eliminates risk of forgetting to update functions
- **JSON-Based Storage**: More compact and flexible than current text file format
- **Type Safety**: Compile-time verification of parameter types

**Risk Mitigation**:
- Test implementation isolated from existing working code
- Gradual rollout preserving existing functionality
- Easy rollback if issues arise
- No changes to proven Animartrix parameter system during testing

### Device State Sync on BLE Connection (TODO)
**Current Limitation**: `syncInitialState()` currently reads component state (defaults) rather than actual device state.

**Problem**: The existing `updateUI()` function in bleControl.h is Animartrix-centric and doesn't understand the broader AuroraPortal program/mode context. When BLE connects, the web interface should query the device for its actual current state, but the ESP32 infrastructure isn't ready for this.

**Root Cause**: Ongoing Animartrix→AuroraPortal integration challenges:
- `updateUI()` only sends `cFxIndex` (animation selector from Animartrix era)
- Missing `PROGRAM` and `MODE` values in BLE communication
- Button 91 handling incomplete (line 932 placeholder in index.html)
- Need program/mode context in ESP32 state management

**Approach**: Complete X-macro parameter refactoring first, then implement proper device state sync:

1. **Phase 1**: Complete X-macro parameter system (current priority)
   - Finish testing CustomA-E parameters
   - Expand to full parameter set
   - Replace manual parameter management

2. **Phase 2**: Refactor ESP32 state management for AuroraPortal context
   - Update `updateUI()` to send both `PROGRAM` and `MODE` 
   - Modify BLE button handling to understand program vs mode context
   - Implement proper state query/response mechanism

3. **Phase 3**: Implement proper device state sync
   - Send state query command on BLE connection (button 91 or string command)
   - ESP32 responds with current `PROGRAM`, `MODE`, and all parameter values
   - `syncInitialState()` applies received state to web interface components
   - Web interface shows actual device state instead of defaults

**Benefits of This Approach**:
- X-macro system will make state sync much easier to implement
- Auto-generated parameter handling eliminates manual sync code
- JSON-based communication simplifies state transfer
- Single source of truth for parameter definitions
