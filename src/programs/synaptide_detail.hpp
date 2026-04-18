//========================================================================================
// synaptide inspired by WaveScene by Knifa Dan (https://github.com/Knifa/matryx-gl) 
//========================================================================================

#pragma once
#include <cmath>
// #include "fl/math.h"  ^^^^
#include "bleControl.h"
#include "profiler.h"

// Forward-declare the LED-mapping arrays defined globally (via matrixMap_*.h
// included by main.cpp). synaptide caches the active pointer once per frame
// to bypass the xyFunc() switch/function-call overhead in its hot loops.
extern const uint16_t progTopDown[NUM_LEDS] PROGMEM;
extern const uint16_t progBottomUp[NUM_LEDS] PROGMEM;
extern const uint16_t serpTopDown[NUM_LEDS] PROGMEM;
extern const uint16_t serpBottomUp[NUM_LEDS] PROGMEM;

namespace synaptide {

    //using namespace fl;

	bool synaptideInstance = false;

	uint16_t (*xyFunc)(uint8_t x, uint8_t y);

    void initMatrix();
    void initSynaptide(uint16_t (*xy_func)(uint8_t, uint8_t));

    static float matrix1[NUM_LEDS];
    static float matrix2[NUM_LEDS];
    static float *matrix = matrix1;
    static float *lastMatrix = matrix2;
    static bool useMatrix1 = true;

    // Spatial variation LUTs - precomputed once per frame/once at init instead
    // of re-evaluated per pixel (saves tens of thousands of trig calls per frame).
    static float sinXVar_LUT[WIDTH];    // per-frame: sinf(x*0.15 + t*0.3)
    static float cosYVar_LUT[HEIGHT];   // per-frame: cosf(y*0.12 + t*0.2)
    static float sinXBias_LUT[WIDTH];   // per-frame: sinf(x*0.08 + t*0.4*cPulse)
    static float cosYBias_LUT[HEIGHT];  // static:    cosf(y*0.09)
    static float sinXTime_LUT[WIDTH];   // per-frame: sinf(t*0.7*cPulse + x*0.02)
    static float neighborSin_LUT[WIDTH + HEIGHT]; // static: sinf(s*0.3), s = nX+nY

    struct FrameTime {
        uint32_t now;
        uint32_t deltaStart;
        uint32_t deltaFrame;
        float t = 0;
        float dt = 0;
    };

    class FrameTimer {
        public:
            FrameTimer();
            const FrameTime tick();

        private:
            uint32_t startTick;
            uint32_t lastTick;
    };

    FrameTimer::FrameTimer() {
        startTick = fl::micros();
        lastTick = startTick;
    }

    const FrameTime FrameTimer::tick() {
        uint32_t now = fl::micros();

        struct FrameTime frameTime;
        frameTime.now = now;
        frameTime.deltaStart = now - startTick;
        frameTime.deltaFrame = now - lastTick;
        frameTime.t = frameTime.deltaStart / 1000000.0f; // Convert to seconds
        frameTime.dt = frameTime.deltaFrame / 1000000.0f; // Convert to seconds

        lastTick = now;
        return frameTime;
    }

    FrameTimer frameTimer;

    inline float randomFactor() {return static_cast<float>(random16(1,65534)) / 65535.0f;}


    //*****************************************************************************
    // Matrix scaling system
    class MatrixScaler {
    private:
        float matrixScale;
        float energyBoostFactor = 1.0f;
        
    public:
        MatrixScaler() {
            // Calculate scaling based on current matrix vs reference (48x32)
            const float REFERENCE_LEDS = 1536.0f;
            const float REFERENCE_PERIMETER = 160.0f;
            
            float currentPerimeter = 2.0f * (WIDTH + HEIGHT);
            float areaScale = NUM_LEDS / REFERENCE_LEDS;
            float perimeterScale = currentPerimeter / REFERENCE_PERIMETER;
            
            // Combined scaling factor (edge effects + total energy)
            matrixScale = fl::sqrtf(areaScale) * (1.0f + 0.3f * (perimeterScale - 1.0f));
        }
        
        // Scaled parameter getters
        // Neighbor threshold: do NOT scale with matrix size (× matrixScale raised
        // the threshold so high on large matrices that blooms couldn't propagate).
        // Invert energyBoostFactor so boost=1.2 makes propagation easier, not harder.
        float getScaledNeighborBase() const {
            return cNeighborBase / energyBoostFactor;
            //return cNeighborBase * matrixScale * energyBoostFactor;
        }

        float getScaledInfluenceBase() const {
            return cInfluenceBase * (1.0f + (1.0f - matrixScale) * 0.2f);
        }

        uint16_t getScaledEntropyRate() const {
            return cEntropyRate * matrixScale; // More frequent on smaller matrices
        }

        // Larger matrices get slightly LESS decay (blooms persist farther before
        // fading). Clamp to keep the rate sane even for extreme sizes.
        float getScaledDecayBase() const {
            float adj = cDecayBase + (matrixScale - 1.0f) * 0.005f;
            return FL_MIN(0.98f, FL_MAX(0.90f, adj));
            //return cDecayBase + (1.0f - matrixScale) * 0.01f;
        }
        
        void setEnergyBoost(float factor) {
            energyBoostFactor = FL_MAX(0.8f, FL_MIN(1.5f, factor)); // Clamp boost range
        }
        
        float getBrightnessScale() const {
            // Compensate for lower energy density in smaller matrices
            return 1.0f + (1.0f - matrixScale) * 0.4f; // Up to 40% brighter for small matrices
        }
        
        float getMatrixScale() const { return matrixScale; }
    };

    //*******************************************************************************
    // Energy monitoring system 
    // (prevents system from decaying into zero-energy (black) state)
    class EnergyMonitor {
    private:
        static const int HISTORY_SIZE = 20;
        float energyHistory[HISTORY_SIZE];
        int historyIndex = 0;
        bool historyFilled = false;
        uint32_t lastBoostTime = 0;
        uint32_t startupTime = 0;
        
    public:
        EnergyMonitor() {
            for(int i = 0; i < HISTORY_SIZE; i++) {
                energyHistory[i] = 0.5f; // Initialize with medium energy
            }
            startupTime = fl::micros();
        }
        
        float getCurrentEnergy(float* matrix) {
            float totalEnergy = 0;
            int activePixels = 0;
            
            for(int i = 0; i < NUM_LEDS; i++) {
                totalEnergy += matrix[i];
                if(matrix[i] > 0.1f) activePixels++;
            }
            
            // Weight both average energy and active pixel ratio
            float avgEnergy = totalEnergy / NUM_LEDS;
            float activeRatio = (float)activePixels / NUM_LEDS;
            return (avgEnergy * 0.7f) + (activeRatio * 0.3f);
        }
        
        void updateHistory(float energy) {
            energyHistory[historyIndex] = energy;
            historyIndex = (historyIndex + 1) % HISTORY_SIZE;
            if(historyIndex == 0) historyFilled = true;
        }
        
        float getAverageEnergy() {
            if(!historyFilled && historyIndex == 0) return 0.5f;
            
            float sum = 0;
            int count = historyFilled ? HISTORY_SIZE : historyIndex;
            for(int i = 0; i < count; i++) {
                sum += energyHistory[i];
            }
            return sum / count;
        }
        
        bool needsEnergyBoost(uint32_t currentTime) {
            // Shorter grace period: allow some energy management after initial seeding
            uint32_t timeSinceStartup = currentTime - startupTime;
            if (timeSinceStartup < 3000000) { // 3 seconds grace period
                return false;
            }
            
            float avgEnergy = getAverageEnergy();
            bool lowEnergy = avgEnergy < 0.18f; // Higher threshold to maintain activity
            bool cooldownExpired = (currentTime - lastBoostTime) > 4000000; // 4 seconds - more frequent
            return lowEnergy && cooldownExpired;
        }
        
        void applyEnergyBoost(float* matrix, uint32_t currentTime) {
            lastBoostTime = currentTime;
            
            // More strategic energy injection - target low areas with medium boost
            int boostCount = FL_MAX(5, NUM_LEDS / 120);
            
            for(int i = 0; i < boostCount; i++) {
                int randI = random16() % NUM_LEDS;
                float currentVal = matrix[randI];
                // Medium boost - enough to sustain but not create geometric artifacts
                float boost = 0.25f + (0.15f * randomFactor());
                matrix[randI] = FL_MIN(1.0f, currentVal + boost);
            }
        }
    };

    MatrixScaler matrixScaler;
    EnergyMonitor energyMonitor;

    //*****************************************************************************
    // Fast power approximation for base in [0,1].
    // IEEE 754 bit-trick: fewer cycles than the old fl::logf-based approximation,
    // and no branch on baseExponent. Caller collapses dynamic exponent by passing
    // (baseExponent + val*0.5f) directly.
    /*
    float fastPowDynamic(float x, float baseExponent) {
        if (x <= 0.0f) return 0.0f;
        if (x >= 1.0f) return 1.0f;
        const float dynamicExp = x * 0.5f;
        float basePower;
        if (baseExponent >= 4.0f) {
            const float x2 = x * x;
            basePower = x2 * x2; // x^4
        } else if (baseExponent >= 3.0f) {
            basePower = x * x * x; // x^3
        } else {
            basePower = x * x; // x^2
        }
        if (x > 0.01f) {
            const float lnApprox = fl::logf(x);
            const float dynamicFactor = 1.0f + (dynamicExp * lnApprox);
            return basePower * dynamicFactor;
        } else {
            return basePower;
        }
    }
    */
    inline float fastpow(float base, float exp) {
        if (base <= 0.0f) return 0.0f;
        if (base >= 1.0f) return 1.0f;
        union { float f; int32_t i; } v = { base };
        v.i = (int32_t)(exp * (v.i - 1065353216) + 1065353216);
        return v.f;
    }

    void setPixel(const int x, const int y, const float r, const float g, const float b) {
        const uint8_t rByte = static_cast<uint8_t>(FL_MAX(0.0f, FL_MIN(1.0f, r)) * 255.0f);
        const uint8_t gByte = static_cast<uint8_t>(FL_MAX(0.0f, FL_MIN(1.0f, g)) * 255.0f);
        const uint8_t bByte = static_cast<uint8_t>(FL_MAX(0.0f, FL_MIN(1.0f, b)) * 255.0f);

        const uint16_t ledIndex = xyFunc(x, y);

        if (ledIndex < NUM_LEDS) {
            leds[ledIndex] = CRGB(rByte, gByte, bByte);
        }
    }


    //*****************************************************************************

    void tick(FrameTime frameTime);

    //void drawMatrix(float *matrix) {
    void drawMatrix(float *matrix, const uint16_t* activeMap) {
        // Hoist frame-constant values out of the per-pixel loop.
        const float brightnessScale = matrixScaler.getBrightnessScale();
        const float bloom4 = 4.0f * cBloomEdge;
        const float bloom3 = 3.0f * cBloomEdge;
        const float bloom2 = 2.0f * cBloomEdge;

        for (int y = 0; y < HEIGHT; y++) {
            const int rowBase = y * WIDTH;
            for (int x = 0; x < WIDTH; x++) {
                // Single mapping lookup per pixel (was two: xyFunc + setPixel->xyFunc).
                //const int i = xyFunc(x, y);
                const uint16_t i = activeMap[rowBase + x];
                const float val = matrix[i];

                // Polynomial approximations for sin/cos over val in [0,1].
                // cos(v) ≈ 1 - v²/2 (max error ~4% at v=1)
                // sin(v) ≈ v - v³/6 (max error <1% at v=1)
                const float val2 = val * val;
                const float cosV = 1.0f - val2 * 0.5f;
                const float sinV = val - val2 * val * (1.0f / 6.0f);

                // Collapse dynamic exponent directly into fastpow's exp argument.
                const float dynExp = val * 0.5f;
                const float r = fastpow(val, bloom4 + dynExp) * cosV * brightnessScale;
                const float g = fastpow(val, bloom3 + dynExp) * sinV * brightnessScale;
                const float b = fastpow(val, bloom2 + dynExp) * brightnessScale;

                // Inlined setPixel (avoids a second xyFunc lookup using the same i).
                //setPixel(x, y, r, g, b);
                const uint8_t rByte = static_cast<uint8_t>(FL_MAX(0.0f, FL_MIN(1.0f, r)) * 255.0f);
                const uint8_t gByte = static_cast<uint8_t>(FL_MAX(0.0f, FL_MIN(1.0f, g)) * 255.0f);
                const uint8_t bByte = static_cast<uint8_t>(FL_MAX(0.0f, FL_MIN(1.0f, b)) * 255.0f);
                if (i < NUM_LEDS) {
                    leds[i] = CRGB(rByte, gByte, bByte);
                }
            }
        }
    }
    
       void initMatrix() {

        // Initialize with natural-looking energy gradients that promote activity
        // Start mostly low with strategic higher-energy areas to seed blooms
        
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                const int i = xyFunc(x, y);
                
                // Base: mostly low energy (below ignition threshold)
                float baseEnergy = 0.05f + (0.1f * randomFactor());
                
                // Create natural-looking energy gradients using distance from multiple seed points
                float maxSeedEnergy = baseEnergy;
                
                // Place several seed areas using natural spacing
                int numSeeds = 4 + (random16() % 4); // 4-7 seeds
                for(int seed = 0; seed < numSeeds; seed++) {
                    // Seed positions using golden ratio for natural distribution
                    float seedAngle = seed * 2.399963f; // Golden angle in radians
                    float seedRadius = 0.3f + (0.4f * randomFactor());
                    float seedX = WIDTH * (0.5f + seedRadius * fl::cosf(seedAngle));
                    float seedY = HEIGHT * (0.5f + seedRadius * fl::sinf(seedAngle));
                    
                    // Distance from this seed
                    float dx = x - seedX;
                    float dy = y - seedY;
                    float dist = fl::sqrtf(dx*dx + dy*dy);
                    float maxDist = fl::sqrtf(WIDTH*WIDTH + HEIGHT*HEIGHT) * 0.4f;
                    
                    if(dist < maxDist) {
                        // Exponential falloff with some noise for organic look
                        float falloff = fl::expf(-dist / (maxDist * 0.3f));
                        float seedEnergy = (0.6f + 0.3f * randomFactor()) * falloff;
                        maxSeedEnergy = FL_MAX(maxSeedEnergy, seedEnergy);
                    }
                }
                
                // Ensure some areas are definitely above ignition threshold
                if(maxSeedEnergy > 0.3f && randomFactor() < 0.7f) {
                    maxSeedEnergy = FL_MAX(0.5f, maxSeedEnergy); // Push viable seeds higher
                }
                
                matrix1[i] = maxSeedEnergy;
                matrix2[i] = maxSeedEnergy;
            }
        }
    }

    void tick(FrameTime frameTime) {

        if (useMatrix1) {
            lastMatrix = matrix1;
            matrix = matrix2;
        } else {
            lastMatrix = matrix2;
            matrix = matrix1;
        }
        useMatrix1 = !useMatrix1;

        // Cache the active mapping table pointer once per frame (was: per-pixel
        // xyFunc() call with switch on cMapping, ~10× per pixel in tick+draw).
        const uint16_t* activeMap;
        switch (cMapping) {
            case 0:  activeMap = progTopDown;    break;
            case 1:  activeMap = progBottomUp;   break;
            case 2:  activeMap = serpTopDown;    break;
            case 3:  activeMap = serpBottomUp;   break;
            default: activeMap = progTopDown;    break;
        }

        // Periodic entropy injection to prevent settling into patterns
        static uint32_t entropyCounter = 0;
        entropyCounter++;

        // Periodically inject some chaos - scaled for matrix size
        if (entropyCounter % matrixScaler.getScaledEntropyRate() == 0) {
            // Reseed random with time-based entropy
            random16_add_entropy(fl::micros());

            // Disturbances with bounds checking
            for (int disturbances = 0; disturbances < 2; disturbances++) {
                int randX = random16() % WIDTH;
                int randY = random16() % HEIGHT;
                //int randI = xyFunc(randX, randY);
                int randI = activeMap[randY * WIDTH + randX];
                if (randI >= 0 && randI < NUM_LEDS) {
                    float currentVal = lastMatrix[randI];
                    float perturbation = 0.05f + 0.15f * randomFactor();
                    lastMatrix[randI] = FL_MIN(1.0f, currentVal + perturbation);
                }
            }
        }

        // Rebuild per-frame spatial-variation LUTs once (was: WIDTH*HEIGHT*3
        // sinf/cosf calls per frame; now: ~4*WIDTH + HEIGHT).
        const float t = frameTime.t;
        const float tPulse = t * cPulse;
        for (int x = 0; x < WIDTH; x++) {
            sinXVar_LUT[x]  = fl::sinf(x * 0.15f + t * 0.3f);
            sinXBias_LUT[x] = fl::sinf(x * 0.08f + tPulse * 0.4f);
            sinXTime_LUT[x] = fl::sinf(tPulse * 0.7f + x * 0.02f);
        }
        for (int y = 0; y < HEIGHT; y++) {
            cosYVar_LUT[y] = fl::cosf(y * 0.12f + t * 0.2f);
        }

        // Hoist frame-constant scalars out of the per-pixel / per-neighbor loops.
        const float decayBase     = matrixScaler.getScaledDecayBase();
        const float neighborBase  = matrixScaler.getScaledNeighborBase();
        const float influenceBase = matrixScaler.getScaledInfluenceBase();

        // Energy accumulators — replaces the separate NUM_LEDS scan in the
        // EVERY_N_MILLISECONDS block below.
        float frameEnergySum = 0.0f;
        int frameActivePixels = 0;

        PROFILE_START("syn_tick");
        for (int y = 0; y < HEIGHT; y++) {
            const int rowBase = y * WIDTH;
            for (int x = 0; x < WIDTH; x++) {
                //const int i = xyFunc(x, y);
                const uint16_t i = activeMap[rowBase + x];
                const float lastValue = lastMatrix[i];

                // Varied decay to break synchronization
                //float spatialVariation = 0.002f * fl::sinf(x * 0.15f + frameTime.t * 0.3f) * fl::cosf(y * 0.12f + frameTime.t * 0.2f);
                float spatialVariation = 0.002f * sinXVar_LUT[x] * cosYVar_LUT[y];
                float decayRate = decayBase + cDecayChaos * randomFactor() + spatialVariation;
                // Clamp decay rate to safe range
                decayRate = FL_MAX(0.88f, FL_MIN(1.0f, decayRate));
                matrix[i] = lastValue * decayRate;

                // Diverse ignition thresholds to create steady flow
                //float spatialBias = 0.015f * fl::sinf(x * 0.08f + frameTime.t * 0.4f * cPulse) * fl::cosf(y * 0.09f);
                //float timeVariation = 0.008f * fl::sinf(frameTime.t * 0.7f * cPulse + x * 0.02f);
                float spatialBias  = 0.015f * sinXBias_LUT[x] * cosYBias_LUT[y];
                float timeVariation = 0.008f * sinXTime_LUT[x];
                float threshold = cIgnitionBase + cIgnitionChaos * randomFactor() + spatialBias + timeVariation;
                // Clamp threshold to safe range
                threshold = FL_MAX(0.08f, FL_MIN(0.28f, threshold));
                if (lastValue <= threshold) {

                    float n = 0;

                    for (int u = -1; u <= 1; u++) {
                        for (int v = -1; v <= 1; v++) {
                            if (u == 0 && v == 0) { continue; }

                            int nX = (x + u + WIDTH) % WIDTH;
                            int nY = (y + v + HEIGHT) % HEIGHT;

                            //const int nI = xyFunc(nX, nY);
                            const uint16_t nI = activeMap[nY * WIDTH + nX];
                            const float nLastValue = lastMatrix[nI];

                            // Varied neighbor thresholds and influence to de-synchronize blooms
                            //float neighborThreshold = matrixScaler.getScaledNeighborBase() + cNeighborChaos * 0.5f + 0.01f * fl::sinf((nX + nY) * 0.3f);
                            float neighborThreshold = neighborBase + cNeighborChaos * randomFactor() + 0.01f * neighborSin_LUT[nX + nY];
                            if (nLastValue >= neighborThreshold) {
                                n += 1;
                                //float influence = matrixScaler.getScaledInfluenceBase() + cInfluenceChaos * 0.5f;
                                float influence = influenceBase + cInfluenceChaos * randomFactor();
                                matrix[i] += nLastValue * influence;
                            }
                        }
                    }

                    if (n > 0) {
                        matrix[i] *= 1.0f / n;
                        // Additional safety clamp
                        matrix[i] = FL_MIN(1.0f, matrix[i]);
                    }
                    // Ensure values stay in valid range
                    matrix[i] = FL_MAX(0.0f, FL_MIN(1.0f, matrix[i]));
                }

                // Accumulate energy stats from final pixel value (folded in to
                // avoid a second pass over NUM_LEDS in the energy monitor).
                const float finalVal = matrix[i];
                frameEnergySum += finalVal;
                if (finalVal > 0.1f) frameActivePixels++;
            }
        }
        PROFILE_END();  // syn_tick

        // Energy monitoring and self-healing system
        EVERY_N_MILLISECONDS(100) {
            //float currentEnergy = energyMonitor.getCurrentEnergy(matrix);
            // Use accumulators from the tick pass (same formula as getCurrentEnergy).
            const float avgEnergy   = frameEnergySum / (float)NUM_LEDS;
            const float activeRatio = (float)frameActivePixels / (float)NUM_LEDS;
            const float currentEnergy = (avgEnergy * 0.7f) + (activeRatio * 0.3f);
            energyMonitor.updateHistory(currentEnergy);

            if (energyMonitor.needsEnergyBoost(frameTime.now)) {
                energyMonitor.applyEnergyBoost(matrix, frameTime.now);
                // Temporarily boost energy transfer for a few frames
                matrixScaler.setEnergyBoost(1.2f);
            } else {
                // Gradually return boost to normal
                matrixScaler.setEnergyBoost(1.0f);
            }
        }

        //drawMatrix(matrix);
        PROFILE_START("syn_draw");
        drawMatrix(matrix, activeMap);
        PROFILE_END();
        //watermelonPlasma(frameTime);

    } // tick

	void runSynaptide() {
        if (synaptideResetRequested) {
            synaptideResetRequested = false;
            initSynaptide(xyFunc);   // full re-init via the cached xy pointer
        }
		auto frameTime = frameTimer.tick();
        tick(frameTime);
        //FastLED.delay(25);
 	}

    void initSynaptide(uint16_t (*xy_func)(uint8_t, uint8_t)) {
        Serial.printf("matrix1 @ %p, matrix2 @ %p\n", matrix1, matrix2);
        synaptideInstance = true;
        xyFunc = xy_func;

        // Random seeding using multiple entropy sources
        random16_set_seed(fl::micros()); //  + analogRead(0)
        random16_add_entropy(fl::millis());

        // Re-initialize timing state after Arduino init (avoids global-ctor
        // ordering issues where fl::micros() ran before the system timer was ready)
        frameTimer = FrameTimer();
        energyMonitor = EnergyMonitor();
        matrixScaler = MatrixScaler();

        // Time-independent spatial LUTs — filled once per synaptide entry.
        for (int y = 0; y < HEIGHT; y++) {
            cosYBias_LUT[y] = fl::cosf(y * 0.09f);
        }
        for (int s = 0; s < WIDTH + HEIGHT; s++) {
            neighborSin_LUT[s] = fl::sinf(s * 0.3f);
        }

        initMatrix();
    }

} // namespace synaptide