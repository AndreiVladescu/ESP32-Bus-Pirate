#include "UtilityService.h"

#include <Arduino.h>
#include <esp_system.h>

uint32_t UtilityService::nowMs() const {
    return ::millis();
}

uint32_t UtilityService::nowUs() const {
    return ::micros();
}

void UtilityService::sleepMs(uint32_t durationMs) const {
    ::delay(durationMs);
}

void UtilityService::sleepUs(uint32_t durationUs) const {
    ::delayMicroseconds(durationUs);
}

uint32_t UtilityService::randomUint32() const {
    return ::esp_random();
}

int32_t UtilityService::randomRange(int32_t minInclusive,int32_t maxExclusive) const {
    if (maxExclusive <= minInclusive) {
        return minInclusive;
    }

    const uint32_t range = static_cast<uint32_t>(
        static_cast<int64_t>(maxExclusive) -
        static_cast<int64_t>(minInclusive)
    );

    const uint32_t offset = static_cast<uint32_t>(
        (static_cast<uint64_t>(randomUint32()) * range) >> 32
    );

    return static_cast<int32_t>(
        static_cast<int64_t>(minInclusive) + offset
    );
}
