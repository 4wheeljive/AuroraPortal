#pragma once
#include <cmath>
#include "bleControl.h"

namespace synaptide {

	bool synaptideInstance = false;

	uint16_t (*xyFunc)(uint8_t x, uint8_t y);

    void sceneSetup();

	void initSynaptide(uint16_t (*xy_func)(uint8_t, uint8_t)) {
		synaptideInstance = true;
		xyFunc = xy_func;

        // Improve random seeding using multiple entropy sources
        random16_set_seed(micros() + analogRead(0));
        random16_add_entropy(millis());

        sceneSetup();
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

    // Matrix scaling and energy management system
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
        
        // Scaled parameter getters - user values remain unchanged
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
            energyBoostFactor = MAX(0.8f, MIN(1.5f, factor)); // Clamp boost range
        }
        
        float getBrightnessScale() const {
            // Compensate for lower energy density in smaller matrices
            return 1.0f + (1.0f - matrixScale) * 0.4f; // Up to 40% brighter for small matrices
        }
        
        float getMatrixScale() const { return matrixScale; }
    };

    // Energy monitoring system
    class EnergyMonitor {
    private:
        static const int HISTORY_SIZE = 20;
        float energyHistory[HISTORY_SIZE];
        int historyIndex = 0;
        bool historyFilled = false;
        uint32_t lastBoostTime = 0;
        
    public:
        EnergyMonitor() {
            for(int i = 0; i < HISTORY_SIZE; i++) {
                energyHistory[i] = 0.5f; // Initialize with medium energy
            }
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
            float avgEnergy = getAverageEnergy();
            bool lowEnergy = avgEnergy < 0.12f;
            bool cooldownExpired = (currentTime - lastBoostTime) > 5000000; // 5 seconds
            return lowEnergy && cooldownExpired;
        }
        
        void applyEnergyBoost(float* matrix, uint32_t currentTime) {
            lastBoostTime = currentTime;
            
            // Strategic energy injection - more points for larger matrices
            int boostCount = MAX(3, NUM_LEDS / 200);
            
            for(int i = 0; i < boostCount; i++) {
                int randI = random16() % NUM_LEDS;
                float currentVal = matrix[randI];
                float boost = 0.2f + (0.3f * randomFactor());
                matrix[randI] = MIN(1.0f, currentVal + boost);
            }
        }
    };

    // Global instances
    MatrixScaler matrixScaler;
    EnergyMonitor energyMonitor;

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
        const uint8_t rByte = static_cast<uint8_t>(MAX(0.0f, MIN(1.0f, r)) * 255.0f);
        const uint8_t gByte = static_cast<uint8_t>(MAX(0.0f, MIN(1.0f, g)) * 255.0f);
        const uint8_t bByte = static_cast<uint8_t>(MAX(0.0f, MIN(1.0f, b)) * 255.0f);

        const uint16_t ledIndex = xyFunc(x, y);

        if (ledIndex < NUM_LEDS) {
            leds[ledIndex] = CRGB(rByte, gByte, bByte);
        }
    }

    void tick(FrameTime frameTime);

    void drawMatrix(float *matrix) {
        float brightnessScale = matrixScaler.getBrightnessScale();
        
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
            const int i = xyFunc(x, y);
            const float val = matrix[i];
            
            // Replace expensive std::pow calls with fast approximations, apply brightness scaling
            float r = fastPowDynamic(val, 4.0f*cEdge) * std::cos(val) * brightnessScale;
            float g = fastPowDynamic(val, 3.0f*cEdge) * std::sin(val) * brightnessScale;
            float b = fastPowDynamic(val, 2.0f*cEdge) * brightnessScale;
            
            setPixel(x, y, r, g, b);
            }
        }
    }

    void sceneSetup() {

        // Initialize both matrices with random values
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                const int i = xyFunc(x, y);
                float randomValue = randomFactor();
                matrix1[i] = randomValue;
                matrix2[i] = randomValue;
            }
        }
    }

    void tick(FrameTime frameTime) {
    
        // Swap the matrix pointers to avoid memory allocation
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
            
            // More conservative disturbances with better bounds checking
            for (int disturbances = 0; disturbances < 2; disturbances++) {
                int randX = random16() % WIDTH;
                int randY = random16() % HEIGHT;
                int randI = xyFunc(randX, randY);
                if (randI >= 0 && randI < NUM_LEDS) {
                    // Smaller, safer perturbations
                    float currentVal = lastMatrix[randI];
                    float perturbation = 0.05f + 0.15f * randomFactor();
                    lastMatrix[randI] = MIN(1.0f, currentVal + perturbation);
                }
            }
        }

        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                const int i = xyFunc(x, y);
                const float lastValue = lastMatrix[i];

                // More varied decay to break synchronization
                float spatialVariation = 0.002f * sinf(x * 0.15f + frameTime.t * 0.3f) * cosf(y * 0.12f + frameTime.t * 0.2f);
                float decayRate = matrixScaler.getScaledDecayBase() + cDecayChaos * randomFactor() + spatialVariation;
                // Clamp decay rate to safe range
                decayRate = MAX(0.88f, MIN(1.0f, decayRate));
                matrix[i] = lastValue * decayRate;

                // More diverse ignition thresholds to create steadier flow
                float spatialBias = 0.015f * sinf(x * 0.08f + frameTime.t * 0.4f * cPulse) * cosf(y * 0.09f);
                float timeVariation = 0.008f * sinf(frameTime.t * 0.7f * cPulse + x * 0.02f);
                float threshold = cIgnitionBase + cIgnitionChaos * randomFactor() + spatialBias + timeVariation;
                // Clamp threshold to safe range  
                threshold = MAX(0.08f, MIN(0.28f, threshold));
                if (lastValue <= threshold) { 
                    
                    float n = 0;

                    for (int u = -1; u <= 1; u++) {
                        for (int v = -1; v <= 1; v++) {
                            if (u == 0 && v == 0) { continue; }

                            // Proper wrapping for negative values
                            int nX = (x + u + WIDTH) % WIDTH;
                            int nY = (y + v + HEIGHT) % HEIGHT;

                            const int nI = xyFunc(nX, nY);
                            const float nLastValue = lastMatrix[nI];

                            // More varied neighbor thresholds to create less synchronized blooms
                            float neighborThreshold = matrixScaler.getScaledNeighborBase() + cNeighborChaos * randomFactor() + 0.01f * sinf((nX + nY) * 0.3f);
                            if (nLastValue >= neighborThreshold) {
                                n += 1;
                                // Slightly more varied influence  
                                float influence = matrixScaler.getScaledInfluenceBase() + cInfluenceChaos * randomFactor();
                                matrix[i] += nLastValue * influence;
                            }
                        }
                    }

                    if (n > 0) { 
                        matrix[i] *= 1.0f / n; 
                        // Additional safety clamp
                        matrix[i] = MIN(1.0f, matrix[i]);
                    }
                    // Ensure values stay in valid range
                    matrix[i] = MAX(0.0f, MIN(1.0f, matrix[i]));
                }
            }
        }

        // Energy monitoring and self-healing system
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

        drawMatrix(matrix);

    }
	
	void runSynaptide() {
		auto frameTime = frameTimer.tick();
        tick(frameTime);
        //FastLED.delay(25);
 	}

} // namespace synaptide