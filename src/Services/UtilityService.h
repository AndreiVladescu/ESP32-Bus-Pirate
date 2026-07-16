#pragma once

#include "Interfaces/IUtilityService.h"

class UtilityService final : public IUtilityService {
public:
    uint32_t nowMs() const override;
    uint32_t nowUs() const override;
    void sleepMs(uint32_t durationMs) const override;
    void sleepUs(uint32_t durationUs) const override;
    uint32_t randomUint32() const override;
    int32_t randomRange(int32_t minInclusive, int32_t maxExclusive) const override;
};
