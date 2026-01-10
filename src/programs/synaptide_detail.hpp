#pragma once
#include <cmath>
#include "fl/math.h" 
#include "bleControl.h"

namespace synaptide {

    using namespace fl;

	bool synaptideInstance = false;

	uint16_t (*xyFunc)(uint8_t x, uint8_t y);

    void initMatrix();

	void initSynaptide(uint16_t (*xy_func)(uint8_t, uint8_t)) {
		synaptideInstance = true;
		xyFunc = xy_func;

        // Random seeding using multiple entropy sources
        random16_set_seed(micros() + analogRead(0));
        random16_add_entropy(millis());

        initMatrix();
	}

    static float matrix1[NUM_LEDS];
    static float matrix2[NUM_LEDS];
    static float *matrix = matrix1;
    static float *lastMatrix = matrix2;
    static bool useMatrix1 = true;

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
        startTick = micros();
        lastTick = startTick;
    }

    const FrameTime FrameTimer::tick() {
        uint32_t now = micros();

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
            matrixScale = sqrt(areaScale) * (1.0f + 0.3f * (perimeterScale - 1.0f));
        }
        
        // Scaled parameter getters
        float getScaledNeighborBase() const {
            return cNeighborBase * matrixScale * energyBoostFactor;
        }
        
        float getScaledInfluenceBase() const {
            return cInfluenceBase * (1.0f + (1.0f - matrixScale) * 0.2f);
        }
        
        uint16_t getScaledEntropyRate() const {
            return cEntropyRate * matrixScale; // More frequent on smaller matrices
        }
        
        float getScaledDecayBase() const {
            // Slightly higher decay for smaller matrices to compensate for scaling
            return cDecayBase + (1.0f - matrixScale) * 0.01f;
        }
        
        void setEnergyBoost(float factor) {
            energyBoostFactor = max(0.8f, min(1.5f, factor)); // Clamp boost range
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
            startupTime = micros();
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
            int boostCount = max(5, NUM_LEDS / 120);
            
            for(int i = 0; i < boostCount; i++) {
                int randI = random16() % NUM_LEDS;
                float currentVal = matrix[randI];
                // Medium boost - enough to sustain but not create geometric artifacts
                float boost = 0.25f + (0.15f * randomFactor());
                matrix[randI] = min(1.0f, currentVal + boost);
            }
        }
    };

    MatrixScaler matrixScaler;
    EnergyMonitor energyMonitor;

    //*****************************************************************************
    // Fast power approximation for the dynamic exponent case
    // Approximates pow(x, base + (x * 0.5)) more efficiently than std::pow
    float fastPowDynamic(float x, float baseExponent) {
  
        if (x <= 0.0f) return 0.0f;
        if (x >= 1.0f) return 1.0f;
        
        const float dynamicExp = x * 0.5f;
        
        // Calculate base powers using simple multiplication
        float basePower;
        if (baseExponent >= 4.0f) {
            const float x2 = x * x;
            basePower = x2 * x2; // x^4
        } else if (baseExponent >= 3.0f) {
            basePower = x * x * x; // x^3
        } else {
            basePower = x * x; // x^2
        }
        
        // Approximate the dynamic exponent effect: pow(x, base + dynamicExp)
        // Using: pow(x, a + b) ≈ pow(x, a) * (1 + b * ln(x)) for small b
        if (x > 0.01f) {
            const float lnApprox = logf(x); // Still need one log call, but much faster than pow
            const float dynamicFactor = 1.0f + (dynamicExp * lnApprox);
            return basePower * dynamicFactor;
        } else {
            return basePower; // Skip dynamic factor for very small x
        }
    } // fastPowDynamic

    void setPixel(const int x, const int y, const float r, const float g, const float b) {
        const uint8_t rByte = static_cast<uint8_t>(max(0.0f, min(1.0f, r)) * 255.0f);
        const uint8_t gByte = static_cast<uint8_t>(max(0.0f, min(1.0f, g)) * 255.0f);
        const uint8_t bByte = static_cast<uint8_t>(max(0.0f, min(1.0f, b)) * 255.0f);

        const uint16_t ledIndex = xyFunc(x, y);

        if (ledIndex < NUM_LEDS) {
            leds[ledIndex] = CRGB(rByte, gByte, bByte);
        }
    }


    //*****************************************************************************

    void tick(FrameTime frameTime);

    void drawMatrix(float *matrix) {
        float brightnessScale = matrixScaler.getBrightnessScale();
        
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
            const int i = xyFunc(x, y);
            const float val = matrix[i];
            
            // Replace expensive std::pow calls with fast approximations, apply brightness scaling
            float r = fastPowDynamic(val, 4.0f*cBloomEdge) * std::cos(val) * brightnessScale;
            float g = fastPowDynamic(val, 3.0f*cBloomEdge) * std::sin(val) * brightnessScale;
            float b = fastPowDynamic(val, 2.0f*cBloomEdge) * brightnessScale;
            
            setPixel(x, y, r, g, b);
            }
        }
    }
    
    /*
    void watermelonPlasma(FrameTime frameTime) {

        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
            float xp = ((x / 128.0f) - 0.5f) * (5.0f + sin(frameTime.t * 0.25)) + sin(frameTime.t * 0.25) * 5.0f;
            float yp = ((y / 128.0f) - 0.5f) * (5.0f + sin(frameTime.t * 0.25)) + cos(frameTime.t * 0.25) * 5.0f;

            float pixel = sin(sin(sin(0.25 * frameTime.t) * xp + cos(0.29 * frameTime.t) * yp + frameTime.t) +
                                sin(sqrt(pow(xp + sin(frameTime.t * 0.25f) * 4.0f, 2) +
                                        pow(yp + cos(frameTime.t * 0.43f) * 4.0f, 2)) +
                                    frameTime.t) -
                                cos(sqrt(pow(xp + cos(frameTime.t * 0.36f) * 6.0f, 2) +
                                        pow(yp + sin(frameTime.t * 0.39f) * 5.3f, 2)) +
                                    frameTime.t));

            float u = pow(cos(9 * pixel + 0.5f * xp + frameTime.t) * 0.5f + 0.5f, 2);
            float v = pow(sin(9 * pixel + 0.5f * yp + frameTime.t) * 0.5f + 0.5f, 2);

            setPixel(x, y, u, v, (u + v) / 2);

            }
        }
    }
    */

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
                    float seedX = WIDTH * (0.5f + seedRadius * cosf(seedAngle));
                    float seedY = HEIGHT * (0.5f + seedRadius * sinf(seedAngle));
                    
                    // Distance from this seed
                    float dx = x - seedX;
                    float dy = y - seedY;
                    float dist = sqrtf(dx*dx + dy*dy);
                    float maxDist = sqrtf(WIDTH*WIDTH + HEIGHT*HEIGHT) * 0.4f;
                    
                    if(dist < maxDist) {
                        // Exponential falloff with some noise for organic look
                        float falloff = expf(-dist / (maxDist * 0.3f));
                        float seedEnergy = (0.6f + 0.3f * randomFactor()) * falloff;
                        maxSeedEnergy = max(maxSeedEnergy, seedEnergy);
                    }
                }
                
                // Ensure some areas are definitely above ignition threshold
                if(maxSeedEnergy > 0.3f && randomFactor() < 0.7f) {
                    maxSeedEnergy = max(0.5f, maxSeedEnergy); // Push viable seeds higher
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
        
        // Periodic entropy injection to prevent settling into patterns
        static uint32_t entropyCounter = 0;
        entropyCounter++;
        
        // Periodically inject some chaos - scaled for matrix size
        if (entropyCounter % matrixScaler.getScaledEntropyRate() == 0) {
            // Reseed random with time-based entropy
            random16_add_entropy(micros());
            
            // Disturbances with bounds checking
            for (int disturbances = 0; disturbances < 2; disturbances++) {
                int randX = random16() % WIDTH;
                int randY = random16() % HEIGHT;
                int randI = xyFunc(randX, randY);
                if (randI >= 0 && randI < NUM_LEDS) {
                    float currentVal = lastMatrix[randI];
                    float perturbation = 0.05f + 0.15f * randomFactor();
                    lastMatrix[randI] = min(1.0f, currentVal + perturbation);
                }
            }
        }

        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                const int i = xyFunc(x, y);
                const float lastValue = lastMatrix[i];

                // Varied decay to break synchronization
                float spatialVariation = 0.002f * sinf(x * 0.15f + frameTime.t * 0.3f) * cosf(y * 0.12f + frameTime.t * 0.2f);
                float decayRate = matrixScaler.getScaledDecayBase() + cDecayChaos * randomFactor() + spatialVariation;
                // Clamp decay rate to safe range
                decayRate = max(0.88f, min(1.0f, decayRate));
                matrix[i] = lastValue * decayRate;

                // Diverse ignition thresholds to create steady flow
                float spatialBias = 0.015f * sinf(x * 0.08f + frameTime.t * 0.4f * cPulse) * cosf(y * 0.09f);
                float timeVariation = 0.008f * sinf(frameTime.t * 0.7f * cPulse + x * 0.02f);
                float threshold = cIgnitionBase + cIgnitionChaos * randomFactor() + spatialBias + timeVariation;
                // Clamp threshold to safe range  
                threshold = max(0.08f, min(0.28f, threshold));
                if (lastValue <= threshold) { 
                    
                    float n = 0;

                    for (int u = -1; u <= 1; u++) {
                        for (int v = -1; v <= 1; v++) {
                            if (u == 0 && v == 0) { continue; }

                            int nX = (x + u + WIDTH) % WIDTH;
                            int nY = (y + v + HEIGHT) % HEIGHT;

                            const int nI = xyFunc(nX, nY);
                            const float nLastValue = lastMatrix[nI];

                            // Varied neighbor thresholds and influence to de-synchronize blooms
                            float neighborThreshold = matrixScaler.getScaledNeighborBase() + cNeighborChaos * randomFactor() + 0.01f * sinf((nX + nY) * 0.3f);
                            if (nLastValue >= neighborThreshold) {
                                n += 1;
                                float influence = matrixScaler.getScaledInfluenceBase() + cInfluenceChaos * randomFactor();
                                matrix[i] += nLastValue * influence;
                            }
                        }
                    }

                    if (n > 0) { 
                        matrix[i] *= 1.0f / n; 
                        // Additional safety clamp
                        matrix[i] = min(1.0f, matrix[i]);
                    }
                    // Ensure values stay in valid range
                    matrix[i] = max(0.0f, min(1.0f, matrix[i]));
                }
            }
        }

        // Energy monitoring and self-healing system
        EVERY_N_MILLISECONDS(100) {
            float currentEnergy = energyMonitor.getCurrentEnergy(matrix);
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

        drawMatrix(matrix);
        //watermelonPlasma(frameTime);

    } // tick

	void runSynaptide() {
		auto frameTime = frameTimer.tick();
        tick(frameTime);
        //FastLED.delay(25);
 	}

} // namespace synaptide