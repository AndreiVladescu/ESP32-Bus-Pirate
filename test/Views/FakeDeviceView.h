#pragma once

#include <string>
#include <vector>

class SPIClass {};

#include "Interfaces/IDeviceView.h"

class FakeDeviceView final : public IDeviceView {
public:
    struct HorizontalSelectionCall {
        std::vector<std::string> options;
        uint16_t selectedIndex = 0;
        std::string description1;
        std::string description2;
    };

    struct AnalogicTraceCall {
        uint8_t pin = 0;
        std::vector<uint8_t> samples;
        uint8_t scale = 0;
    };

    struct LogicTraceCall {
        uint8_t pin = 0;
        std::vector<uint8_t> samples;
        uint8_t scale = 0;
    };

    struct WaterfallCall {
        std::string title;
        float minFrequency = 0.0f;
        float maxFrequency = 0.0f;
        std::string unit;
        int index = 0;
        int count = 0;
        int level = 0;
    };

    SPIClass spi;
    std::vector<PinoutConfig> shownConfigs;
    std::vector<std::string> topBarTitles;
    std::vector<HorizontalSelectionCall> horizontalSelectionCalls;
    std::vector<LogicTraceCall> logicTraceCalls;
    std::vector<AnalogicTraceCall> analogicTraceCalls;
    std::vector<WaterfallCall> waterfallCalls;

    void initialize() override {}
    SPIClass& getSharedSpiInstance() override { return spi; }
    void* getScreen() override { return nullptr; }
    void logo() override {}
    void welcome(TerminalTypeEnum&, std::string&) override {}
    void show(PinoutConfig& config) override { shownConfigs.push_back(config); }
    void loading() override {}
    void adapterMode(const std::string&, const std::string&, const std::vector<std::string>&) override {}
    void clear() override {}
    void drawLogicTrace(uint8_t pin, const std::vector<uint8_t>& samples, uint8_t scale) override {
        logicTraceCalls.push_back({pin, samples, scale});
    }
    void drawAnalogicTrace(uint8_t pin, const std::vector<uint8_t>& samples, uint8_t scale) override {
        analogicTraceCalls.push_back({pin, samples, scale});
    }

    void drawWaterfall(const std::string& title,
                       float minFrequency,
                       float maxFrequency,
                       const char* unit,
                       int index,
                       int count,
                       int level) override {
        waterfallCalls.push_back({title, minFrequency, maxFrequency, unit == nullptr ? "" : unit, index, count, level});
    }
    void setRotation(uint8_t) override {}
    void setBrightness(uint8_t brightness) override { brightnessValue = brightness; }
    uint8_t getBrightness() override { return brightnessValue; }
    void topBar(const std::string& title, bool, bool) override {
        topBarTitles.push_back(title);
    }

    void horizontalSelection(const std::vector<std::string>& options,
                             uint16_t selectedIndex,
                             const std::string& description1,
                             const std::string& description2) override {
        horizontalSelectionCalls.push_back({
            options,
            selectedIndex,
            description1,
            description2
        });
    }

private:
    uint8_t brightnessValue = 0;
};
