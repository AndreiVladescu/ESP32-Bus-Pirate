#pragma once

#include <cstdint>
#include <string>
#include <vector>
#if __has_include("Vendors/FastLED.h")
#include "Vendors/FastLED.h"
#else
#include <FastLED.h>
#endif

class ILedService {
public:
    virtual ~ILedService() = default;

    virtual void configure(uint8_t dataPin, uint8_t clockPin, uint16_t length,
                           const std::string& protocol, uint8_t brightness) = 0;
    virtual void release() = 0;
    virtual void fill(const CRGB& color) = 0;
    virtual void set(uint16_t index, const CRGB& color) = 0;
    virtual void resetLeds() = 0;
    virtual void runAnimation(const std::string& type) = 0;
    virtual bool isAnimationRunning() const = 0;
    virtual std::vector<std::string> getSingleWireProtocols() = 0;
    virtual std::vector<std::string> getSpiChipsets() = 0;
    virtual std::vector<std::string> getSupportedProtocols() = 0;
    virtual std::vector<std::string> getSupportedAnimations() = 0;
    virtual int getMaxLeds() = 0;
    virtual CRGB parseStringColor(const std::string& input) = 0;
    virtual CRGB parseHtmlColor(const std::string& input) = 0;
};
