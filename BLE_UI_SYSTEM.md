# AuroraPortal BLE/UI Control System

## Overview

The AuroraPortal uses a Web Bluetooth (BLE) bridge between a browser-based UI (`index.html`) and an ESP32 controller (`bleControl.h`). All communication is JSON-based (except raw button values), flowing over 4 BLE characteristics. The UI is built entirely with vanilla Web Components that auto-render controls based on the active "visualizer."

---

## 1. BLE Transport Layer

### 1.1 Four Characteristics

| Characteristic | UUID suffix | Purpose | Send format | Receive (notify) format |
|---|---|---|---|---|
| **Button** | `...1214` | Programs, modes, presets, triggers | Raw `Uint8Array([value])` | Raw string of the value |
| **Checkbox** | `...2214` | Boolean toggles | `{"id":"cxN","val":bool}` | Same JSON echoed back |
| **Number** | `...3214` | Slider/dropdown values | `{"id":"inParam","val":float}` | Same JSON echoed back |
| **String** | `...4214` | State sync, future use | `{"id":"..","val":".."}` | Same JSON echoed back |

All four characteristics support READ, WRITE, and NOTIFY.

### 1.2 Communication Flow

```
UI Action --> write to characteristic --> ESP32 BLE callback
  --> processButton/Checkbox/Number/String()
  --> applies value to global `cParam` variable
  --> sendReceipt*() notifies back on same characteristic
  --> UI handleChange handler updates display
```

The "receipt" pattern ensures the UI always reflects the device's actual accepted value.

### 1.3 Bus Parameters (extended Number format)

For per-bus audio parameters, the Number characteristic carries an extra `"bus"` field:
```json
{"id": "inThreshold", "val": 0.35, "bus": 0}
```
On the ESP32 side, `processNumber()` checks for `busId >= 0` and delegates to a function-pointer callback (`setBusParam -> handleBusParam`) which routes to `busA`/`busB`/`busC` struct fields directly.

On the UI side, `sendBusParamCharacteristic(paramId, value, busId)` handles this.

---

## 2. Visualizer Concept

A **visualizer** is the fundamental unit of the control system. It identifies the current visual output and determines which parameter controls to display.

### 2.1 Two Forms

| Form | Example | Structure |
|---|---|---|
| **Modeless program** | `"dots"`, `"cube"` | Just the program name (lowercase) |
| **Program-mode combination** | `"radii-octopus"`, `"animartrix-polarwaves"` | `programName-modeName` |

### 2.2 Programs and Modes

Programs are enumerated in both C++ and JS as parallel constants:

```
C++ enum:  RAINBOW=0, WAVES=1, BUBBLE=2, DOTS=3, FXWAVE2D=4, RADII=5,
           ANIMARTRIX=6, TEST=7, SYNAPTIDE=8, CUBE=9, HORIZONS=10, AUDIOTEST=11
```

Only 4 programs have modes: WAVES (2), RADII (5), ANIMARTRIX (10), AUDIOTEST (10).

`MODE_COUNTS[]` array (parallel index by program number) stores mode counts, with `0` for modeless programs.

### 2.3 Visualizer Name Construction

`VisualizerManager::getVisualizerName(PROGRAM, MODE)` on ESP32 and `getCurrentVisualizer()` in JS both produce the same string by:
1. Lowercasing the program name
2. If the program has modes, appending `-modeName` (also lowercase)

---

## 3. Variable Naming Conventions

The system uses a strict prefix-based naming convention that threads through the entire stack:

| Context | Pattern | Example | Where Used |
|---|---|---|---|
| **C++ global variable** | `c` + PascalCase | `cZoom`, `cSpeed` | `bleControl.h` globals |
| **BLE parameter ID** | `in` + PascalCase | `inZoom`, `inSpeed` | JSON `"id"` field over BLE |
| **JS registry key** | camelCase | `zoom`, `speed` | `VISUALIZER_PARAMETER_REGISTRY` |
| **HTML attribute** | `parameter-id="inZoom"` | `parameter-id="inZoom"` | `<control-slider>` elements |
| **Visualizer param list** | camelCase | `["zoom","speed"]` | `VISUALIZER_PARAMS` / C++ arrays |
| **Checkbox ID** | `cx` + number | `cx5`, `cx10` | `sendCheckboxCharacteristic` |

### Conversion helpers (JS):
- `getParameterLabel(paramName)` - `"zoom"` -> `"Zoom"` (capitalize first letter)
- `getParameterId(paramName)` - `"zoom"` -> `"inZoom"` (add `in` prefix + capitalize)

### X-Macro Convention (C++):
The `PARAMETER_TABLE` X-macro entries use PascalCase names (e.g., `Zoom`). The macro expansions:
- `c##parameter` -> `cZoom` (the global variable)
- `"in" #parameter` -> `"inZoom"` (the BLE parameter ID)
- `#parameter` -> `"Zoom"` (for JSON key in presets/state)

---

## 4. Parameter Registries

### 4.1 Parallel Registries (C++ and JS must stay in sync)

**Visualizer parameters** - which sliders to show for each visualizer:
- C++: `VISUALIZER_PARAM_LOOKUP[]` array of `VisualizerParamEntry` structs
- JS: `VISUALIZER_PARAMS` object (`{"radii-octopus": ["zoom","angle","speedInt"], ...}`)

**Parameter ranges** - min/max/step/default for each parameter:
- JS only: `VISUALIZER_PARAMETER_REGISTRY` object
- C++: defaults live in the `PARAMETER_TABLE` X-macro (but not ranges)

**Audio parameters** - global audio controls (not per-bus):
- JS: `AUDIO_PARAMETER_REGISTRY` + `AUDIO_PARAMS`
- C++: `AUDIO_PARAMS[]` array (defined but `sendAudioState()` not yet functional)

**Bus parameters** - per-bus beat/envelope settings:
- JS: `BUS_PARAMETER_REGISTRY` (threshold, minBeatInterval, expDecayFactor, rampAttack, rampDecay, peakBase)
- C++: `Bus` struct fields in `audioTypes.h`

### 4.2 X-Macro Parameter Table

The `PARAMETER_TABLE` in `bleControl.h` is the single source of truth for all `cParam` variables. It drives:

1. **`captureCurrentParameters()`** - serializes all `cParam` values into a JSON object (for preset save)
2. **`applyCurrentParameters()`** - deserializes JSON into `cParam` variables + sends UI receipts (for preset load)
3. **`processNumber()`** - auto-generated `if (receivedID == "inParam") { cParam = value; }` chains
4. **`sendVisualizerState()`** - iterates the current visualizer's param list and reads matching `cParam` values via `strcasecmp`

Each entry is `X(type, PascalName, defaultValue)`:
```cpp
X(float, Zoom, 1.0f)       // creates cZoom, matches "inZoom", serializes as "Zoom"
X(float, Speed, 1.0f)      // creates cSpeed, matches "inSpeed", serializes as "Speed"
```

---

## 5. Web Components

### 5.1 Component Architecture

Components fall into two categories:

**"One-off" controls** - placed directly in HTML with explicit attributes:
| Component | Tag | Description |
|---|---|---|
| `BLEConnectionPanel` | `<ble-connection-panel>` | Connect/disconnect buttons + status |
| `ControlButton` | `<control-button>` | Sends a button value; attrs: `label`, `data-my-number` |
| `ControlCheckbox` | `<control-checkbox>` | Boolean toggle; attrs: `label`, `data-my-number`, `checked`/`unchecked` |
| `ControlSlider` | `<control-slider>` | Sends number value; attrs: `label`, `parameter-id`, `min`, `max`, `step`, `default-value` |
| `ControlDropdown` | `<control-dropdown>` | Index selector; attrs: `label`, `parameter-id`; options from inner text |

**Auto-rendering grouped components** - dynamically generate their content:
| Component | Tag | Description |
|---|---|---|
| `ProgramSelector` | `<program-selector>` | Grid of program buttons, highlights current |
| `ModeSelector` | `<mode-selector>` | Grid of mode buttons, updates when program changes |
| `ParameterSettings` | `<parameter-settings>` | Auto-renders sliders from `VISUALIZER_PARAMS[currentVisualizer]` |
| `AudioSettings` | `<audio-settings>` | Auto-renders sliders from `AUDIO_PARAMS["audio"]` |
| `BusSettings` | `<bus-settings>` | Renders 3 groups (A/B/C) of sliders from `BUS_PARAMETER_REGISTRY` |
| `LayerSelector` | `<layer-selector>` | 9 layer checkboxes (sends `cxLayer1`..`cxLayer9`) |
| `Presets` | `<preset-controls>` | Save (101-110) / Load (121-130) preset button grids |

### 5.2 Dynamic Visibility (`data-visualizers`)

Any HTML element (including one-off controls and wrapper `<div>`s) can have a `data-visualizers` attribute:
```html
<control-checkbox label="Rotate waves" data-my-number="10" data-visualizers="waves" checked>
<control-button label="Trigger" data-my-number="160" data-visualizers="fxwave2d, animartrix">
```

`updateDynamicControls()` runs whenever the visualizer changes and:
1. Queries ALL elements with `[data-visualizers]`
2. Parses the comma-separated patterns
3. Uses `visualizerMatches()` which supports both exact match (`"cube"`) and prefix match (`"animartrix"` matches `"animartrix-polarwaves"`)
4. Sets `display: ''` or `display: 'none'`

### 5.3 Rendering Lifecycle

```
BLE Connect
  --> syncInitialState()
      --> sends button 92
      --> ESP32 sendVisualizerState()
          --> builds JSON: {program, mode, parameters: {paramName: value, ...}}
          --> sends via string characteristic as {"id":"visualizerState","val":"..."}
      --> applyReceivedString() parses it
          --> applyReceivedButton(program)  -> updates ProgramSelector, ModeSelector
          --> applyReceivedButton(mode+20)  -> updates ModeSelector highlight
          --> applyReceivedNumber() for each param -> updates ParameterSettings sliders

Program/Mode change (user clicks):
  --> selectProgram()/selectMode() sends button value
  --> ESP32 processButton() sets PROGRAM/MODE, sends receipt
  --> applyReceivedButton() updates:
      1. ProgramSelector.updateDisplayProgram()
      2. ModeSelector.updateModes() (re-renders mode buttons for new program)
      3. ParameterSettings.updateParameters() (re-renders sliders for new visualizer)
      4. updateDynamicControls() (show/hide one-off controls)
```

---

## 6. Preset System

### 6.1 File Format (LittleFS on ESP32)

Presets are saved as `/preset_N.json`:
```json
{
  "programNum": 6,
  "modeNum": 2,
  "parameters": {
    "Speed": 1.5,
    "Zoom": 0.8,
    ...all PARAMETER_TABLE entries...
  }
}
```

### 6.2 Save/Load Flow

- **Save**: Button values 101-120 -> `savePreset(N)` -> `captureCurrentParameters()` captures ALL `cParam` values
- **Load**: Button values 121-140 -> `loadPreset(N)` -> sets `PROGRAM`/`MODE`, then `applyCurrentParameters()` sets each `cParam` and sends `sendReceiptNumber("inParam", value)` for UI sync

Note: `loadPreset()` is called twice in the current code (line 900-901 in bleControl.h) -- likely a bug.

---

## 7. State Sync Mechanisms

### 7.1 Existing: Visualizer State Sync

`sendVisualizerState()` (triggered by button 92):
1. Gets current visualizer name
2. Looks up the parameter list for that visualizer via `VISUALIZER_PARAM_LOOKUP`
3. Uses X-macro + `strcasecmp` to match each param name to its `cParam` variable
4. Builds JSON with program, mode, and parameter values
5. Sends via string characteristic as `{"id":"visualizerState","val":"<JSON>"}`

The UI's `applyReceivedString()` handler parses this and:
- Updates program/mode selectors via `applyReceivedButton()`
- Updates parameter sliders via `applyReceivedNumber()` for each param

### 7.2 `sendAudioState()` -- Not Yet Functional

`sendAudioState()` exists but is a copy of `sendVisualizerState()` -- it still uses `VISUALIZER_PARAM_LOOKUP` instead of `AUDIO_PARAMS`, so it doesn't actually send audio parameter values. There is no button code wired to trigger it, and no corresponding UI handler to process audio state responses.

---

## 8. Known Issues and Gaps

### 8.1 Initial Sync Timing/Rendering Bugs

**Problem**: When the UI connects and requests state (button 92), the following race conditions exist:

1. **`applyReceivedButton(program)` triggers `ProgramSelector.updateDisplayProgram()`**, which calls `modeSelector.updateModes()`. `updateModes()` calls `render()` which rebuilds the mode buttons, then `updateDisplayMode(0)`. But the actual mode from the device hasn't been applied yet -- that comes in the *next* `applyReceivedButton(mode+20)` call. So the mode briefly shows as 0 regardless of actual state.

2. **`ParameterSettings.updateParameters()` is called during program update**, which triggers `render()` rebuilding all sliders with registry defaults. Then `applyReceivedNumber()` tries to update those sliders with device values. But if the render hasn't completed (DOM not ready), the slider queries may fail silently.

3. **`ParameterSettings.render()` always uses `config.default`** for slider initial values (line ~2093), not the actual device values. The device values arrive later via `applyReceivedNumber()`, but if the parameter isn't in the current visualizer's rendered set at that moment, the update is lost.

4. **Line 2150: `this.updateParameterValues;`** is a no-op (missing parentheses and arguments) -- this appears to be an incomplete attempt to fix the sync issue.

5. **Line 2183: `input.addEventListener('keydown', (e) => {updateSliderValues`** -- stray `updateSliderValues` text before the function body. This is a syntax issue that may cause a JS error (appears in both ParameterSettings and AudioSettings).

### 8.2 Audio Parameter Sync -- Not Implemented

**Current state**:
- Audio parameters (noiseGateOpen, noiseGateClose, audioGain, avLevelerTarget) render with defaults from `AUDIO_PARAMETER_REGISTRY` but are never synced from device state
- `sendAudioState()` is a broken copy of `sendVisualizerState()` -- it loops over visualizer params instead of audio params
- No button code triggers `sendAudioState()`
- No `applyReceivedString` handler for an audio state message
- The audio `cParam` globals (e.g., `cAudioGain`, `cNoiseGateOpen`) exist in the X-macro table but aren't included in `sendVisualizerState()`'s scope

**What's needed**:
1. Fix `sendAudioState()` to iterate `AUDIO_PARAMS[]` instead of `VISUALIZER_PARAM_LOOKUP`
2. Wire a button code (e.g., 93) to trigger it
3. Add handling in `applyReceivedString()` for an `"audioState"` message
4. Call it during `syncInitialState()`

### 8.3 Bus Parameter Sync -- Not Implemented

**Current state**:
- `BusSettings` renders 3 groups of sliders with defaults from `BUS_PARAMETER_REGISTRY`
- Sending works: `sendBusParamCharacteristic()` sends `{"id","val","bus"}`, ESP32 `handleBusParam()` applies to correct `busA/B/C` struct
- But there's no mechanism to read current bus values back from the device
- Bus params live on `myAudio::busA/B/C` struct instances, not in the `cParam` global namespace, so they're outside the X-macro system entirely
- Presets (`captureCurrentParameters`/`applyCurrentParameters`) don't capture bus params -- they only capture `cParam` globals

**What's needed**:
1. A `sendBusState()` function that serializes all 3 buses' params into JSON
2. A UI handler to parse and apply bus state to `BusSettings` sliders
3. Integration with preset save/load (either extend `PARAMETER_TABLE` or add separate bus capture)

### 8.4 Layer State Sync -- Not Implemented

`Layer1`..`Layer9` booleans exist in `bleControl.h` and are set via `cxLayer1`..`cxLayer9` checkboxes, but:
- No mechanism to query current layer state from device
- `LayerSelector` always initializes all layers as `true`
- Not included in preset save/load

### 8.5 Checkbox State Sync -- Partial

`applyReceivedCheckbox()` tries to find checkboxes by `data-my-number` attribute, but:
- Only works for `ControlCheckbox` elements, not `LayerSelector` checkboxes
- No bulk-sync mechanism exists (checkboxes are never queried from device state)
- Checkbox values aren't in presets

### 8.6 Minor Bugs

- **`MODE_COUNTS` mismatch**: C++ has `{0, 2, 0, 0, 0, 5, 10, 0, 0, 0, 0, 10}` (AUDIOTEST=10 modes), JS has `[0, 2, 0, 0, 0, 5, 10, 0, 0, 0, 0, 9]` (AUDIOTEST=9 modes)
- **`RAINBOW_PARAMS` count mismatch**: C++ lookup says count=4 but the array is empty (`{}`)
- **Typo in JS**: `"audiotest-radiamspectrum"` should be `"audiotest-radialspectrum"` (line ~943)
- **Missing hyphen in C++**: `"audiotestflbeatdetection"` should be `"audiotest-flbeatdetection"` (bleControl.h line 193)
- **Double `loadPreset()` call**: `bleControl.h` line 900-901 calls `loadPreset()` twice
- **`sendAudioState()` is dead code**: Exact copy of `sendVisualizerState()`, never called, and wouldn't work for audio params anyway

---

## 9. Architecture Diagrams

### 9.1 Data Flow: UI -> Device

```
[control-slider "inZoom"]
  |
  v
sendNumberCharacteristic("inZoom", 1.5)
  |
  v  (BLE write: {"id":"inZoom","val":1.5})
NumberCharacteristicCallbacks::onWrite()
  |
  v
processNumber("inZoom", 1.5)
  |-- sendReceiptNumber("inZoom", 1.5)  --> notify --> UI handleNumberCharacteristicChange
  |-- X-macro match: cZoom = 1.5
```

### 9.2 Data Flow: Bus Param UI -> Device

```
[bus-settings slider: busA threshold]
  |
  v
sendBusParamCharacteristic("inThreshold", 0.35, 0)
  |
  v  (BLE write: {"id":"inThreshold","val":0.35,"bus":0})
NumberCharacteristicCallbacks::onWrite()
  |
  v
processNumber("inThreshold", 0.35, busId=0)
  |-- sendReceiptNumber("inThreshold", 0.35)
  |-- setBusParam(0, "inThreshold", 0.35)
        |
        v
  handleBusParam() --> busA.threshold = 0.35
```

### 9.3 Data Flow: State Sync (Connect)

```
syncInitialState()
  |
  v
sendButtonCharacteristic(92)
  |
  v
processButton(92) --> sendVisualizerState()
  |
  v  builds: {"program":6,"mode":2,"parameters":{"speed":1.5,"zoom":0.8,...}}
sendReceiptString("visualizerState", jsonString)
  |
  v
applyReceivedString()
  |-- applyReceivedButton(6)       --> ProgramSelector, ModeSelector, ParameterSettings update
  |-- applyReceivedButton(2 + 20)  --> ModeSelector highlight
  |-- for each param:
        applyReceivedNumber({"id":"inSpeed","val":1.5})  --> ParameterSettings slider update
```

---

## 10. File Reference

| File | Role |
|---|---|
| `index.html` | Complete web UI: styles, HTML structure, BLE transport, all Web Components |
| `src/bleControl.h` | BLE setup, characteristics, callbacks, processButton/Number/Checkbox, X-macro table, presets, state sync |
| `src/audio/audioTypes.h` | `Bus` struct, `BusPreset`, `AudioFrame`, bin/bus definitions |
| `src/audio/audioProcessing.h` | `handleBusParam()`, `initBus()`, audio pipeline, `setBusParam` registration |
