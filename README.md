# AuroraPortal

FastLED-based LED matrix controller for ESP32 microcontrollers featuring multiple animation patterns, Bluetooth Low Energy (BLE) control, and a web-based control interface.

## Platform

- **Microcontroller**: ESP32 (Seeed XIAO ESP32S3)
- **Framework**: Arduino (via PlatformIO)
- **Platform**: ESP32 (Espressif32)
- **Build System**: PlatformIO
- **Filesystem**: LittleFS

## Hardware Requirements

### LED Matrix Configurations

Two hardware configurations supported (toggle via `#define BIG_BOARD` in `src/main.cpp`):

- **Small Board (Default)**: 24×24 matrix (576 WS2812B LEDs), single data pin (GPIO2)
- **Big Board**: 32×48 matrix (1536 WS2812B LEDs), 3 parallel data pins (GPIO2/3/4)

### Connection

- Data pins: GPIO2 (required), GPIO3 & GPIO4 (big board only)
- USB connection for programming and serial debugging

## Dependencies

### Core Libraries

- **[FastLED](https://github.com/FastLED/FastLED)** - LED control and animation engine
- **[ArduinoJson](https://arduinojson.org/)** (^7.4.2) - JSON parsing for BLE communication and preset storage

### Built-in ESP32 Libraries

- **BLE** (Bluetooth Low Energy) - Wireless control interface
- **LittleFS** - File system for preset persistence
- **Preferences** - Non-volatile storage

## Features

### Animation Programs

10 distinct animation programs with multiple modes:

- **Rainbow** - Classic rainbow matrix animation
- **Waves** - Pride waves with palette rotation (2 modes)
- **Bubble** - Noise-based fluid simulation
- **Dots** - Oscillator-based particle system with trails
- **fxWave2d** - Complex wave physics simulation
- **Radii** - Polar coordinate effects (5 modes: octopus, flower, lotus, radial, lollipop)
- **Animartrix** - FastLED Animartrix integration (10 modes)
- **Test** - Development/testing program
- **Synaptide** - Neural-inspired wave propagation
- **Cube** - 3D cube projection

### Control System

- **Web Bluetooth Interface** (`index.html`) - Direct browser-to-ESP32 control
- **BLE Characteristics** - Four-characteristic interface for buttons, checkboxes, numbers, and strings
- **Preset System** - Save/load parameter configurations to LittleFS
- **Real-time Parameters** - Dynamic control of speed, zoom, scale, color, and pattern-specific settings

### Parameter Management

- **X-macro based system** - Centralized parameter definitions with auto-generated handlers
- **JSON presets** - Portable preset files stored in LittleFS
- **Dynamic UI** - Web interface automatically shows relevant parameters for each visualizer

## Installation

### Prerequisites

- [PlatformIO](https://platformio.org/) installed (via VS Code extension or CLI)
- USB cable for ESP32 connection

### Build and Upload

```bash
# Build the project
pio run

# Upload to ESP32
pio run -t upload

# Upload web interface to LittleFS (first time setup)
pio run -t uploadfs

# Open serial monitor
pio run -t monitor
```

## Usage

### Web Interface

1. Open `index.html` in a Web Bluetooth-compatible browser (Chrome, Edge)
2. Click "Connect" and select your ESP32 device
3. Select animation programs and modes
4. Adjust parameters with sliders
5. Save/load presets

**Note**: Web Bluetooth requires HTTPS or localhost for security

### Serial Debugging

Enable debug output by setting `debug = true` in `src/main.cpp:58`

## Configuration

### Board Selection

Edit `src/main.cpp` line 60:

```cpp
#define BIG_BOARD    // For 32×48 matrix with 3 data pins
#undef BIG_BOARD     // For 24×24 matrix with 1 data pin (default)
```

### Matrix Mapping

Custom XY coordinate mappings in `src/reference/matrixMap_*.h` translate logical positions to physical LED indices. Four mapping modes available:
- Top-down progressive
- Top-down serpentine
- Bottom-up progressive
- Bottom-up serpentine

### BLE Handle Limit

For >4 BLE characteristics, increase `numHandles` in:
```
.platformio/packages/framework-arduinoespressif32/libraries/BLE/src/BLEServer.h
```
Set to 60 for 7 characteristics (see `src/bleControl.h:6-9`)

## Project Structure

```
AuroraPortal/
├── src/
│   ├── main.cpp                    # Main loop and program coordination
│   ├── bleControl.h                # BLE server and parameter management
│   ├── programs/                   # Animation programs
│   │   ├── {name}.hpp              # Program interface
│   │   └── {name}_detail.hpp       # Program implementation
│   └── reference/
│       ├── matrixMap_*.h           # LED coordinate mappings
│       └── palettes.h              # Color palette definitions
├── index.html                      # Web Bluetooth control interface
├── platformio.ini                  # Build configuration
└── CLAUDE.md                       # Development guide
```

## Credits

### Pattern Implementations

- **Pride waves** - Mark Kriegsman ([Pride2015](https://gist.github.com/kriegsman/964de772d64c502760e5))
- **Color waves** - Mark Kriegsman ([ColorWavesWithPalettes](https://gist.github.com/kriegsman/8281905786e8b2632aeb))
- **Bubble** - Stepko ([Soap](https://editor.soulmatelights.com/gallery/1626-soap)), Stefan Petrick
- **Dots** - Stefan Petrick ([FunkyClouds](https://github.com/FastLED/FastLED/tree/master/examples/FunkyClouds))
- **fxWave2d** - Zach Vorhies ([FastLED FxWave2d](https://github.com/FastLED/FastLED/tree/master/examples/FxWave2d))
- **Radii patterns** - Stepko, Sutaburosu, Stefan Petrick
- **Animartrix** - Stefan Petrick, adapted by Netmindz and Zach Vorhies
- **Synaptide** - Knifa Dan ([WaveScene](https://github.com/Knifa/matryx-gl))
- **Cube** - Fluffy-Wishbone-3497 ([AI-generated](https://www.reddit.com/r/FastLED/comments/1nvuzjg/claude_does_like_to_code_fastled/))

### Infrastructure

- **BLE control** - Jason Coons ([esp32-fastled-ble](https://github.com/jasoncoon/esp32-fastled-ble))
- **Special thanks** - Marc Miller (u/marmilicious)

## License

See individual source files for attribution and licensing information for specific patterns and algorithms.
