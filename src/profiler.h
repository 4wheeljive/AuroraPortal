#ifndef PROFILER_H
#define PROFILER_H

#include <Arduino.h>

// Simple microsecond-level profiler for Arduino ESP32
class SimpleProfiler {
private:
    struct TimingData {
        const char* name;
        unsigned long totalTime;
        unsigned long callCount;
        unsigned long maxTime;
    };

    static const int MAX_SECTIONS = 20;
    TimingData sections[MAX_SECTIONS];
    int sectionCount = 0;

    unsigned long startTime;
    int currentSection = -1;

public:
    SimpleProfiler() {
        for (int i = 0; i < MAX_SECTIONS; i++) {
            sections[i].name = nullptr;
            sections[i].totalTime = 0;
            sections[i].callCount = 0;
            sections[i].maxTime = 0;
        }
    }

    void start(const char* sectionName) {
        // Find or create section
        currentSection = -1;
        for (int i = 0; i < sectionCount; i++) {
            if (sections[i].name == sectionName) {
                currentSection = i;
                break;
            }
        }

        if (currentSection == -1 && sectionCount < MAX_SECTIONS) {
            currentSection = sectionCount++;
            sections[currentSection].name = sectionName;
            sections[currentSection].totalTime = 0;
            sections[currentSection].callCount = 0;
            sections[currentSection].maxTime = 0;
        }

        startTime = micros();
    }

    void end() {
        if (currentSection >= 0) {
            unsigned long elapsed = micros() - startTime;
            sections[currentSection].totalTime += elapsed;
            sections[currentSection].callCount++;
            if (elapsed > sections[currentSection].maxTime) {
                sections[currentSection].maxTime = elapsed;
            }
        }
    }

    void printStats() {
        Serial.println("\n=== Profiling Results ===");
        Serial.println("Section                    | Avg(us) | Max(us) | Calls  | Total(ms)");
        Serial.println("--------------------------------------------------------");

        for (int i = 0; i < sectionCount; i++) {
            if (sections[i].callCount > 0) {
                unsigned long avgTime = sections[i].totalTime / sections[i].callCount;
                char buffer[80];
                snprintf(buffer, sizeof(buffer),
                    "%-25s | %7lu | %7lu | %6lu | %9.2f",
                    sections[i].name,
                    avgTime,
                    sections[i].maxTime,
                    sections[i].callCount,
                    sections[i].totalTime / 1000.0
                );
                Serial.println(buffer);
            }
        }
        Serial.println("=============================\n");
    }

    void reset() {
        for (int i = 0; i < sectionCount; i++) {
            sections[i].totalTime = 0;
            sections[i].callCount = 0;
            sections[i].maxTime = 0;
        }
    }
};

// Global profiler instance
extern SimpleProfiler profiler;

// Convenience macros
#define PROFILE_START(name) profiler.start(name)
#define PROFILE_END() profiler.end()

#endif // PROFILER_H
