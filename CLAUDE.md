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
- **src/programs/fxWave2d.hpp** + **src/programs/fxWave2d_detail.hpp**: Complex FastLED fx engine wave physics
- **src/programs/radii.hpp** + **src/programs/radii_detail.hpp**: Polar coordinate mathematics (4 modes)
- **src/programs/animartrix.hpp** + **src/programs/animartrix_detail.hpp**: FastLED Animartrix integration

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

**Current Hybrid State**: Animartrix became "program 6" with its cFxIndex functioning as modes within AuroraPortal's broader architecture.

**Integration Challenges**:
- Most current bleControl.h code and UI roots came from Animartrix
- Functions like `updateUI()` are still Animartrix-centric and don't understand broader AuroraPortal program/mode context
- Need to refactor numerous functions to work within new/broader AuroraPortal context

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
- One-line addition for new parameters

**Critical Lesson Learned**: Initial X-macro implementation attempt (previous session) was fundamentally flawed because:
1. **Lost sight of original goal**: X-macros should automate code generation, not replace entire architecture
2. **Eliminated file persistence**: Completely removed LittleFS storage, defeating the purpose of presets
3. **Single parameter capture**: Only stored individual parameter values instead of collections
4. **Architecture confusion**: Used X-macros as foundation instead of automation tool

**Key Insight**: The real value is using JSON as the preset container (replacing struct) while using X-macros only to automate the repetitive code that handles those JSON presets.

## Recent Enhancements

### Visualizer Concept (Implemented)
**Definition**: A "visualizer" refers to either (a) a specific program/mode combination (e.g., "radii::octopus") or (b) a modeless program (e.g., "dots").

**Implementation**: JavaScript-based system in index.html that maps applicable parameters to each visualizer.

### Dynamic Parameter System (Implemented)
**Architecture**:
- **ControlSlider**: Standalone slider component (renamed from ParameterSlider) for manual deployment
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

### NEW PARAMETER/PRESET MANAGEMENT SYSTEM "PPMS" (Implemented - Session 250823)

#### Core Architecture
- **JSON-based preset containers** storing multiple parameters + visualizer name
- **LittleFS file persistence** (e.g., `/custom_preset_1.json`, `/custom_preset_2.json`) 
- **X-macros for code generation only** - automating repetitive declarations and functions
- **Visualizer name identification** instead of index numbers for future-proofing

#### File Persistence Functions
```cpp
bool savexPreset(int presetNumber);  // Saves to /custom_preset_N.json
bool loadxPreset(int presetNumber, String& loadedVisualizer);  // Returns visualizer name
```

#### System Flow
1. **Save**: Button press → captures current `cParameter` values + visualizer name → saves to JSON file
2. **Load**: Button press → reads JSON file → applies parameters with change detection → returns loaded visualizer name for UI sync

#### Future Enhancements Planned
1. **Improve visualizer names**: Replace `getCurrentVisualizerName()` with proper program/mode → name mapping  
2. **UI synchronization**: Use returned `loadedVisualizer` to set correct program/mode in web interface
3. **Expand parameter set**: Add more AuroraPortal-specific parameters to CUSTOM_PARAMETER_TABLE
4. **Eventually replace**: Migrate existing Animartrix struct-based presets to this system

### ENUM-BASED PROGRAM/MODE SYSTEM (Implemented - Session 250825)

#### Solution Implemented
**ESP32 Side (bleControl.h)**:
```cpp
enum Program : uint8_t {
    RAINBOW = 0, WAVES = 1, BUBBLE = 2, DOTS = 3, 
    FXWAVE2D = 4, RADII = 5, ANIMARTRIX = 6
};

// PROGMEM arrays for memory efficiency
const char* const PROGRAM_NAMES[] PROGMEM = {...};
const char* const WAVES_MODES[] PROGMEM = {palette_str, pride_str};
const char* const RADII_MODES[] PROGMEM = {...};
const char* const ANIMARTRIX_MODES[] PROGMEM = {...};

class VisualizerManager {
    static String getVisualizerName(int programNum, int mode = -1);
    // Returns dash-format names: "rainbow", "waves-palette", "radii-octopus", etc.
};
```

**JavaScript Side (index.html)**:
```javascript
const Programs = { RAINBOW: 0, WAVES: 1, BUBBLE: 2, ... };
const PROGRAM_NAMES = ["RAINBOW", "WAVES", "BUBBLE", ...];
const WAVES_MODES = ["PALETTE", "PRIDE"];
function getModesForProgram(programIndex) { /* coordinated logic */ }
```

#### Key Improvements
1. **Single Source of Truth**: Enum values provide stable, named constants
2. **Coordinated Systems**: Both ESP32 and JavaScript use parallel structures  
3. **Dash Format**: Standardized on `program-mode` format (e.g., `waves-palette`)
4. **Memory Efficient**: PROGMEM storage for ESP32 flash constraints
5. **Type Safe**: Named constants (`Programs.WAVES`) instead of magic numbers
6. **Future Proof**: Easy to add programs/modes by updating constants

#### Implementation Details
**Phase 1 - ESP32 Foundation**:
- Added enum Program with explicit uint8_t sizing for BLE compatibility
- Created PROGMEM string arrays for program/mode names  
- Implemented VisualizerManager::getVisualizerName() with dash format output
- Updated getCurrentVisualizerName() to use VisualizerManager instead of placeholder
- Added debug output showing proper visualizer names in serial monitor

**Phase 2 - JavaScript Integration**:
- Added parallel JavaScript constants matching C++ definitions
- Updated ProgramSelector to use PROGRAM_NAMES instead of hardcoded array
- Updated ModeSelector to use getModesForProgram() helper function
- Replaced hardcoded switch statements with structured lookups

**Phase 3 - VISUALIZER_PARAMS Migration**:
- Updated all keys from double-colon to dash format (`waves::palette` → `waves-palette`)  
- Fixed ANIMARTRIX modes to match C++ definitions (including `coolwaves` rename)
- Updated getCurrentVisualizer() to use constants and produce dash format
- Ensured parameter system works with new visualizer names

#### Critical Bug Fixes
**NaN Issues on BLE Connection**:
- Problem: `parseInt(null)` returns `NaN`, and `NaN || 0` still equals `NaN`
- Fixed: Added explicit `isNaN()` checks in ProgramSelector/ModeSelector constructors
- Fixed: Added NaN handling in getCurrentVisualizer() to prevent crashes
- Result: Clean BLE connections without console errors

#### Naming Conflict Resolution
- **WAVES enum vs WAVES mode**: C++ namespaces conflicted, renamed animartrix mode to `COOLWAVES`
- **String type conflicts**: Used Arduino `String result = ""; result += ...` pattern to avoid FastLED fl::string conflicts

#### Lessons Learned
1. **Git Branch Workflow**: Successfully used VSCode GUI for branch creation, work, and merging
2. **Error Handling**: Always validate parsed integers with `isNaN()` checks
3. **String Construction**: Use `+=` operator pattern for Arduino String types
4. **Memory Management**: PROGMEM is essential for ESP32 enum-based systems
5. **Coordination**: Parallel data structures require disciplined maintenance but provide robust foundation

#### Current Status
- ✅ **Fully Implemented and Tested**: Enum system working on both ESP32 and JavaScript sides
- ✅ **Merged to Main**: progMode-refactor branch successfully merged and cleaned up
- ✅ **PPMS Integration Ready**: New VisualizerManager provides proper names for preset system
- ✅ **Robust Error Handling**: BLE connection issues resolved
- ✅ **Foundation Complete**: Ready for next phase of AuroraPortal integration

### Device State Sync on BLE Connection (NEXT PRIORITY)
**Current Limitation**: `syncInitialState()` reads component defaults rather than actual device state when BLE connects.

**Updated Context**: With enum-based system complete, we now have the foundation needed to implement proper device state synchronization. The VisualizerManager can convert between program/mode indices and visualizer names bidirectionally.

**Implementation Approach**:

**Phase 1**: Extend ESP32 BLE Communication  
- Update `updateUI()` to send current `PROGRAM` and `MODE` values (not just legacy `cFxIndex`)
- Implement device state query/response using button 91 or string characteristic
- Add VisualizerManager::parseVisualizer() for setting PROGRAM/MODE from visualizer names

**Phase 2**: JavaScript Device State Query
- Modify `syncInitialState()` to send device state query on BLE connection  
- Parse ESP32 response to get actual current program/mode/parameters
- Update UI components to reflect real device state instead of defaults

**Phase 3**: PPMS Integration Enhancement
- Use parseVisualizer() in loadxPreset() to set PROGRAM/MODE from loaded visualizer names
- Complete the TODO items in bleControl.h lines 577-581 for preset loading
- Enable full preset system that syncs device state with loaded presets

**Benefits After Implementation**:
- Web interface shows actual device state on connection
- Preset loading properly switches programs/modes  
- No more disconnect between device reality and UI display
- Foundation ready for full PPMS migration from struct-based presets

