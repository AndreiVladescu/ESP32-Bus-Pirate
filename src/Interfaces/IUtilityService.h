#pragma once

#include <cstdint>

class IUtilityService {
public:
    virtual ~IUtilityService() = default;

    virtual uint32_t nowMs() const = 0;
    virtual uint32_t nowUs() const = 0;
    virtual void sleepMs(uint32_t durationMs) const = 0;
    virtual void sleepUs(uint32_t durationUs) const = 0;
    virtual uint32_t randomUint32() const = 0;
    virtual int32_t randomRange(int32_t minInclusive, int32_t maxExclusive) const = 0;
};
