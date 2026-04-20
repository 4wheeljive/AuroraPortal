# Animartrix Timer Redesign — Parameters & Units First

## Context

The animartrix timing system is hard to work with because:
1. Effective speed is split across two opaque numbers (`master_speed × ratio`) spanning different orders of magnitude per effect
2. Raw array indices (`ratio[3]`, `move.noise_angle[7]`) carry no semantic meaning
3. Offsets are raw numbers (0, 100, 200...) with no intuitive unit

The redesign starts with the **parameters and units** — making the numbers human-friendly — then fits structures around them.

## The Math Chain (unchanged)

```
getTime() returns milliseconds
runtime = getTime() * master_speed * speed_factor

linear[i]      = (runtime + offset[i]) * ratio[i]     → unbounded ramp
radial[i]      = fmod(linear[i], 2π)                   → periodic angle [0, 2π]
directional[i] = sin(radial[i])                        → amplitude [-1, +1]
noise_angle[i] = π × (1 + pnoise(linear[i], 0, 0))    → organic angle [0, 2π]
```

One full `radial` cycle completes when `linear` increases by 2π.
Cycle period = `2π / (effective_speed × 1000)` seconds, where `effective_speed = master_speed × ratio`.

## New Parameter System

### Timer Speed: Normalized 0–10 Scale

Replace the `master_speed × ratio` product with a single **speed level** per timer. The mapping is logarithmic (each step ≈ 2.7× faster), covering the full range of current effects:

| Level | Description | Effective speed | Cycle period |
|-------|-------------|-----------------|-------------|
| 0 | Frozen | 0 | ∞ |
| 1 | Glacial | 0.00002 | ~5 min |
| 2 | Very slow | 0.000054 | ~1.9 min |
| 3 | Slow | 0.000145 | ~43 sec |
| 4 | Moderate | 0.00039 | ~16 sec |
| 5 | Medium | 0.00105 | ~6 sec |
| 6 | Brisk | 0.00283 | ~2.2 sec |
| 7 | Fast | 0.00761 | ~0.8 sec |
| 8 | Very fast | 0.0205 | ~0.3 sec |
| 9 | Rapid | 0.0551 | ~0.11 sec |
| 10 | Maximum | 0.148 | ~0.04 sec |

**Mapping function:**
```cpp
// Convert 0-10 speed level to the internal effective_speed value
// that replaces (master_speed * ratio) in the existing formula.
float speedFromLevel(float level) {
    if (level <= 0.0f) return 0.0f;
    constexpr float base = 0.00002f;   // speed at level 1
    constexpr float k    = 2.69f;      // growth factor per level
    return base * powf(k, level - 1.0f);
}
```

**Current effects mapped to the new scale** (approximate, with default cSpeed/cRatBase):
- Polar_Waves timers: effective ~0.00125 → **level ~4.8** (moderate-medium)
- CK6 timers: effective ~0.00025–0.00057 → **level ~3.6–4.3** (slow-moderate)
- Spiralus slow timers: effective ~0.000023–0.00022 → **level ~1.1–3.4**
- Spiralus fast timers: effective ~0.00165–0.0033 → **level ~5.1–5.8**
- Caleido1 fast timer (slot 4): effective ~0.0018 → **level ~5.4**

The scale covers everything. Non-integer levels (e.g., 4.8) are fine — the scale is continuous.

**Global tempo knob (cSpeed):** Applied as a multiplier to the effective speed, same as today. `cSpeed = 2.0` doubles all timer speeds; `cSpeed = 0.5` halves them. This is separate from the per-timer level.

### Timer Offset: Fraction of Cycle (0.0 to 1.0)

Replace raw offset numbers with a **cycle fraction**. An offset of 0.33 means "start 1/3 of a cycle ahead." For 3 evenly-spaced layers: offsets 0.0, 0.33, 0.67.

**Conversion to internal offset:**
The existing formula is `linear[i] = (runtime + offset[i]) * effective_speed`. For the offset to shift by exactly `fraction` of a cycle (where one cycle = 2π in linear space):

```cpp
// Convert cycle fraction to the internal offset value
float offsetFromFraction(float fraction, float effectiveSpeed) {
    if (effectiveSpeed <= 0.0f) return 0.0f;
    // One full cycle in linear space = 2π
    // offset * effectiveSpeed = fraction * 2π
    return (fraction * 2.0f * PI) / effectiveSpeed;
}
```

Note: offset depends on speed. This is correct — a "1/3 cycle" phase shift for a slow timer is a longer real-time delay than for a fast timer. The conversion is computed once at setup, not per pixel.

### Summary: What the Artist Works With

**Per timer, you specify two numbers:**
- **Speed level** (0–10): how fast this timer runs. Continuous, logarithmic. 5 = medium (~6 sec cycle).
- **Phase fraction** (0.0–1.0): how far ahead of the reference this timer starts. 0.33 = one-third of a cycle.

**Globally:**
- **cSpeed** (UI knob): scales all timer speeds uniformly.

**Example — Polar_Waves with new parameters:**
```
Timer 0 (layer 1): speed 4.8,  phase 0.0
Timer 1 (layer 2): speed 4.9,  phase 0.0
Timer 2 (layer 3): speed 5.0,  phase 0.0
```
vs the current:
```
master_speed = 0.5
ratio[0] = 0.0025, ratio[1] = 0.0027, ratio[2] = 0.0031
```

**Example — Spiralus with new parameters:**
```
Timer 0: speed 5.1,  phase 0.0      (layer 1 offset_y)
Timer 1: speed 5.4,  phase 0.17     (layer 1 z, layer 2 offset_y)
Timer 2: speed 5.7,  phase 0.33     (layer 2 z, layer 3 offset_y)
Timer 3: speed 1.5,  phase 0.5      (aux: directional modulator)
Timer 4: speed 2.5,  phase 0.67     (aux: directional modulator)
Timer 5: speed 1.2,  phase 0.0      (noise_angle for layer 1)
Timer 6: speed 1.1,  phase 1.0      (noise_angle cross)
Timer 7: speed 0.9,  phase 0.0      (noise_angle for layer 2)
Timer 8: speed 1.15, phase 0.0      (noise_angle for layer 3)
```
vs the current:
```
master_speed = 0.0011
ratio = [1.5, 2.3, 3.0, 0.05, 0.2, 0.03, 0.025, 0.021, 0.027]
offset = [0, 100, 200, 300, 400, 500, 600]
```

## What Still Needs Design

### Named timer slots
The index soup problem (`move.radial[3]` vs `move.noise_angle[7]`) is separate from the units problem. Possible approaches:
- Named constants: `constexpr uint8_t T_ANGLE_L1 = 0;` then `move.radial[T_ANGLE_L1]`
- Layer struct with `.slot` fields (as in the earlier design, but lighter weight)
- Comments-only approach (just document which index is which)

### numActiveTimers
Limits the `calculate_timers` loop to only compute timers actually in use. Worth carrying forward — simple optimization.

### Layer struct (optional)
Could bundle named timer indices + static render params per layer. But the user wants to evaluate whether this adds value after the parameter system is settled.

## Files to Modify

- `src/programs/animartrix_detail.hpp` — add `speedFromLevel()`, `offsetFromFraction()`, modify `calculate_timers()` or add a new setup path, convert one effect as proof of concept

## Verification

1. Add `speedFromLevel()` and `offsetFromFraction()` helper functions
2. Convert Polar_Waves to use new parameters
3. Visually compare: the animation should look identical (same effective speeds)
4. Unconverted effects must still work (backward compatible)
5. Adjust speed levels and confirm intuitive response (level 3 = noticeably slower than level 5, etc.)
