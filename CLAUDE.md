# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

AuroraPortal is a FastLED-based LED matrix controller for ESP32 microcontrollers featuring multiple animation patterns, BLE control, and a web interface. The system creates dynamic LED displays with real-time parameter control via Web Bluetooth.

## Development Commands

### Build and Flash
- `pio run` - Build the project
- `pio run -t upload` - Build and upload to ESP32
- `pio run -t uploadfs` - Upload filesystem (LittleFS files)
- `pio run -t monitor` - Open serial monitor
- `pio run -t clean` - Clean build files

### Development Environment
- **Platform**: PlatformIO with Arduino framework for ESP32
- **Target board**: Seeed XIAO ESP32S3
- **Filesystem**: LittleFS for preset storage
- **Key libraries**: FastLED, ArduinoJson, ESP32 BLE

## Core Architecture

### Hardware Configuration
Two LED matrix configurations controlled by `#define BIG_BOARD` in main.cpp:60:
- **Small board (default)**: 24x24 matrix (576 LEDs), single data pin (GPIO2)
- **Big board**: 32x48 matrix (1536 LEDs), 3 parallel data pins (GPIO2/3/4)

Custom XY coordinate mappings via lookup tables in `src/reference/matrixMap_*.h` translate logical coordinates to physical LED indices.

### Program/Mode System

**Program-based architecture** (bleControl.h:34-47):
- Programs are main animation types (RAINBOW, WAVES, BUBBLE, DOTS, FXWAVE2D, RADII, ANIMARTRIX, TEST, SYNAPTIDE, CUBE)
- Some programs have multiple modes (e.g., WAVES has "palette" and "pride" modes; RADII has 5 modes)
- A **visualizer** is either a program-mode combination (e.g., "radii-octopus") or a modeless program (e.g., "dots")

**Program file structure**:
- Each program: `src/programs/{name}.hpp` (interface) + `src/programs/{name}_detail.hpp` (implementation)
- Circular dependency pattern: main.cpp → program.hpp → program_detail.hpp → bleControl.h
- Programs requiring XY mapping use function pointer pattern to access main.cpp's XY functions
- Complex programs (fxWave2d) use dynamic object creation with XYMap parameter passing

### Parameter/Preset Management System (PPMS)

**X-macro based parameter system** (bleControl.h:445-514):
- Single `PARAMETER_TABLE` defines all parameters with type, name, and default value
- Auto-generates `captureCurrentParameters()` and `applyCurrentParameters()` functions
- Parameters follow naming convention: `cParameter` (current value), with `inParameter` for UI display
- Enables adding new parameters by editing table in just a few locations

**JSON-based preset storage**:
- Presets saved to LittleFS as `/preset_N.json` files
- Structure: `{"programNum": N, "modeNum": M, "parameters": {...}}`
- Visualizer identification by name (not index) for future-proofing
- `savePreset()` and `loadPreset()` functions handle persistence

### BLE Control System

**Four-characteristic BLE interface** (bleControl.h:338-350):
- Button characteristic: Program/mode selection, preset operations
- Checkbox characteristic: Boolean flags (layer toggles, display on/off)
- Number characteristic: Numeric parameters (speed, zoom, brightness, etc.)
- String characteristic: Text data exchange

**Communication pattern**:
- JSON messages for structured data: `{"id": "parameterName", "val": value}`
- Receipt confirmation via `sendReceipt*()` functions
- Bidirectional state synchronization between ESP32 and web interface

### Web Interface (index.html)

**Custom web components** using vanilla JavaScript:
- `ble-connection-panel`: BLE device connection management
- `animation-selector`: Program/mode selection
- `parameter-settings`: Dynamic parameter sliders based on active visualizer
- `preset-controls`: Save/load preset interface
- `layer-controls`: Layer visibility toggles

**BLE State Management**:
- Global `window.BLEState` object tracks connection and device state
- Components show loading states when disconnected
- Auto-sync device state on connection via `syncInitialState()`

**Visualizer parameter mappings** (bleControl.h:116-165):
- PROGMEM arrays define which parameters apply to each visualizer
- `VISUALIZER_PARAM_LOOKUP` table mirrors JavaScript `VISUALIZER_PARAMS`
- Parameter controls dynamically rendered based on active visualizer

### Key Integration Points

**XY Mapping coordination**:
- `myXY(x, y)` in main.cpp:164 handles general coordinate mapping
- `XYMapAuroraPortal()` creates FL::XYMap objects for FastLED fx engine
- Mapping selection via `cMapping` parameter (0-3: different orientations)

**Animation loop flow**:
1. `loop()` in main.cpp calls current program's render function
2. Program reads `c*` parameters from bleControl.h
3. FastLED.show() outputs to LED strips
4. BLE callbacks process incoming parameter changes

**Parameter flow**:
- Web interface → BLE Number characteristic → `processNumber()` callback → `c*` variables
- Preset load → `applyCurrentParameters()` → bulk parameter updates with receipts
- Parameter change → `sendReceiptNumber()` → web interface updates UI

## Development Notes

- **Board configuration**: Verify `BIG_BOARD` define matches hardware before building
- **BLE handle limits**: ESP32 BLE requires increasing `numHandles` in BLEServer.h for >4 characteristics (see bleControl.h:6-9)
- **Serial debugging**: Set `debug = true` in main.cpp:58
- **Ignored files**: `/ignore` folder contains experimental/deprecated code - do not reference
- **Web Bluetooth security**: Requires HTTPS or localhost for browser access
- **Mapping files**: Located in `src/reference/` directory, included via platformio.ini build flags

## Important Reminders

- Always prefer editing existing files over creating new ones
- Do not modify or reference files in `/ignore` folder
- Program enum values (bleControl.h:34) must match array indices
- Mode counts (bleControl.h:113) must match actual mode arrays
- Parameter names in X-macro table are case-sensitive and generate `c` prefix automatically
