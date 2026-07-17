#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Interfaces/ILedService.h"

class FakeLedService final : public ILedService {
public:
    struct Configuration {
        uint8_t dataPin = 0;
        uint8_t clockPin = 0;
        uint16_t length = 0;
        std::string protocol;
        uint8_t brightness = 0;
    };

    struct SetCall {
        uint16_t index = 0;
        CRGB color;
    };

    Configuration lastConfiguration;
    std::vector<CRGB> fillCalls;
    std::vector<SetCall> setCalls;
    std::vector<std::string> animationCalls;
    std::vector<std::string> stringColorInputs;
    std::vector<std::string> htmlColorInputs;
    std::vector<std::string> singleWireProtocols = {"ws2812", "sk6812"};
    std::vector<std::string> spiChipsets = {"apa102"};
    std::vector<std::string> animations = {"blink", "rainbow", "chase", "cycle", "wave"};
    CRGB stringColor{1, 2, 3};
    CRGB htmlColor{4, 5, 6};
    uint32_t configureCalls = 0;
    uint32_t releaseCalls = 0;
    uint32_t resetCalls = 0;
    int maxLeds = 100;
    bool animationRunning = false;

    void configure(uint8_t dataPin, uint8_t clockPin, uint16_t length,
                   const std::string& protocol, uint8_t brightness) override {
        ++configureCalls;
        lastConfiguration = {dataPin, clockPin, length, protocol, brightness};
    }

    void release() override { ++releaseCalls; }
    void fill(const CRGB& color) override { fillCalls.push_back(color); }
    void set(uint16_t index, const CRGB& color) override { setCalls.push_back({index, color}); }
    void resetLeds() override { ++resetCalls; }
    void runAnimation(const std::string& type) override { animationCalls.push_back(type); }
    bool isAnimationRunning() const override { return animationRunning; }
    std::vector<std::string> getSingleWireProtocols() override { return singleWireProtocols; }
    std::vector<std::string> getSpiChipsets() override { return spiChipsets; }

    std::vector<std::string> getSupportedProtocols() override {
        auto result = singleWireProtocols;
        result.insert(result.end(), spiChipsets.begin(), spiChipsets.end());
        return result;
    }

    std::vector<std::string> getSupportedAnimations() override { return animations; }
    int getMaxLeds() override { return maxLeds; }

    CRGB parseStringColor(const std::string& input) override {
        stringColorInputs.push_back(input);
        return stringColor;
    }

    CRGB parseHtmlColor(const std::string& input) override {
        htmlColorInputs.push_back(input);
        return htmlColor;
    }
};
