

● Cellular Automata Key Parameters Guide

  1. DECAY SYSTEM

  float decayRate = 0.95 + 0.04 * randomFactor() + spatialVariation;

  Primary Controls:
  - 0.95 (Base decay) - Most Important
    - Higher = patterns persist longer, more buildup       
    - Lower = faster fade, more dynamic
    - Range: 0.92-0.98
  - 0.04 (Decay randomization) [DecayChaos]
    - Higher = more chaotic decay, breaks synchronization
    - Lower = more uniform, predictable patterns
    - Range: 0.01-0.08

  Spatial Variation:
  float spatialVariation = 0.002f * sinf(x * 0.15f +       
  frameTime.t * 0.3f) * cosf(y * 0.12f + frameTime.t * 0.2f);
  - 0.002f (Magnitude) - strength of spatial decay variation
  - 0.15f, 0.12f (Spatial frequency) - size of decay zones
  - 0.3f, 0.2f (Time drift) - how fast zones move

  2. IGNITION THRESHOLD

  float threshold = 0.16 + 0.05 * randomFactor() + spatialBias + timeVariation;

  Primary Controls:
  - 0.16 (Base threshold) - Very Important [IgnitionBase]
    - Lower = easier ignition, more blooms, faster activity
    - Higher = harder ignition, fewer blooms, slower activity
    - Range: 0.12-0.25
  - 0.05 (Threshold randomization) [IgnitionChaos]
    - Higher = more varied ignition timing
    - Range: 0.02-0.08

  Spatial/Time Bias:
  float spatialBias = 0.015f * sinf(x * 0.08f + frameTime.t * 0.4f) * cosf(y * 0.09f);
  float timeVariation = 0.008f * sinf(frameTime.t * 0.7f + x * 0.02f);
  - 0.015f, 0.008f (Magnitudes) - strength of spatial/time variation
  - 0.08f, 0.09f (Spatial freq) - size of ignition preference zones
  - 0.4f, 0.7f (Time freq) - speed of zone movement/pulsing

  3. NEIGHBOR INFLUENCE 

  float neighborThreshold = 0.48 + 0.06 * randomFactor() + 0.01f * sinf((nX + nY) * 0.3f);

  Primary Controls:
  - 0.48 (Base neighbor threshold) - Important [NeighborBase]
    - Lower = easier neighbor ignition, more spreading     
    - Higher = harder neighbor ignition, more isolated blooms
    - Range: 0.4-0.6
  - 0.06 (Neighbor randomization) [NeighborChaos]
    - Higher = more varied spreading patterns
    - Range: 0.02-0.10

  Influence Strength:
  float influence = 0.7 + 0.35 * randomFactor();
  - 0.7 (Base influence) - strength of neighbor effect   [InfluenceBase]  
  - 0.35 (Influence randomization) - variation in spreading strength   [InfluenceChaos]

  4. ENTROPY INJECTION

  if (entropyCounter % 180 == 0) {

  Controls:
  - 180 (Injection frequency) - frames between chaos injections
    - Lower = more frequent disruption, more chaotic       
    - Higher = less frequent disruption, more stable patterns
    - Range: 60-300
  - Perturbation strength:  float perturbation = 0.05f + 0.15f * randomFactor();     
  - 0.05f, 0.15f - magnitude of chaos injection

  5. VISUAL RENDERING (drawMatrix function)

  fastPowDynamic(val, 4.0f) * std::cos(val)  // Red channel    
  fastPowDynamic(val, 3.0f) * std::sin(val)  // Green channel      
  fastPowDynamic(val, 2.0f)                  // Blue channel       

  Power exponents (4.0f, 3.0f, 2.0f):
  - Higher = more contrast, sharper edges
  - Lower = softer, more gradual transitions

● Quick Tuning Recipes:

  For less pulsing, steadier flow:
  - Increase base decay: 0.95 → 0.96-0.97 (DecayBase)
  - Decrease base threshold: 0.16 → 0.14-0.15 (IgnitionBase)
  - Increase decay randomization: 0.04 → 0.06 (DecayChaos)

  For more dramatic blooms:
  - Decrease base decay: 0.95 → 0.93-0.94
  - Increase base threshold: 0.16 → 0.18-0.20
  - Lower neighbor threshold: 0.48 → 0.45 (NeighborBase)

  For more chaotic/less predictable:
  - Increase all randomization values by ~50%
  - Decrease entropy injection frequency: 180 → 120        
  - Increase spatial variation magnitudes

  For calmer, more organic:
  - Reduce randomization values by ~30%
  - Increase entropy injection frequency: 180 → 240        
  - Reduce spatial variation magnitudes

● The most impactful parameters to experiment with first are:

  1. Base decay rate (0.95) - controls overall activity level
  2. Base ignition threshold (0.16) - controls bloom frequency
  3. Base neighbor threshold (0.48) - controls spreading behavior

  Start by adjusting these three by small amounts (±0.01-0.02) to see 
  how they affect the flow and pulsing patterns!
