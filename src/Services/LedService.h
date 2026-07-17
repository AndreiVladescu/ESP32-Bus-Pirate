#pragma once

#include <FastLED.h>
#include <string>
#include <vector>
#include <map>
#include "Interfaces/ILedService.h"

class LedService : public ILedService {
public:
    LedService();

    void configure(uint8_t dataPin, uint8_t clockPin, uint16_t length, const std::string& protocol, uint8_t brightness) override;
    void release() override;
    void fill(const CRGB& color) override;
    void set(uint16_t index, const CRGB& color) override;
    void resetLeds() override;
    void runAnimation(const std::string& type) override;
    bool isAnimationRunning() const override;
    std::vector<std::string> getSingleWireProtocols() override;
    std::vector<std::string> getSpiChipsets() override;
    std::vector<std::string> getSupportedProtocols() override;
    std::vector<std::string> getSupportedAnimations() override;
    int getMaxLeds() override;
    CRGB parseStringColor(const std::string& input) override;
    CRGB parseHtmlColor(const std::string& input) override;
private:
    CRGB* leds = nullptr;
    uint16_t ledCount = 0;
    bool usesClock = false;
    bool animationRunning = false;

    // Allocate a maximum number of LEDs once
    // to avoid dynamic allocation issues with FastLED
    static const uint16_t MAX_LEDS = 128;
};