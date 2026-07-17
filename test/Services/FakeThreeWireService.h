#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Interfaces/IThreeWireService.h"

class FakeThreeWireService final : public IThreeWireService {
public:
    struct Configuration {
        uint8_t cs = 0;
        uint8_t sk = 0;
        uint8_t di = 0;
        uint8_t doPin = 0;
        int16_t model = 0;
        bool org8 = false;
    };

    std::vector<Configuration> configurations;
    uint32_t endCalls = 0;
    std::vector<std::string> supportedModels = {"93xx46", "93xx56"};

    void configure(uint8_t cs, uint8_t sk, uint8_t di, uint8_t doPin,
                   int16_t model = 66, bool org8 = false) override {
        configurations.push_back({cs, sk, di, doPin, model, org8});
    }

    void end() override { ++endCalls; }

    uint16_t read16(uint16_t) override { return 0; }
    uint8_t read8(uint16_t) override { return 0; }
    void write16(uint16_t, uint16_t) override {}
    void write8(uint16_t, uint8_t) override {}
    void writeAll(uint16_t) override {}

    void erase(uint16_t) override {}
    void eraseAll() override {}

    std::vector<uint8_t> dump8() override { return {}; }
    std::vector<uint16_t> dump16() override { return {}; }
    uint16_t sizeBytes() const override { return 0; }

    void writeEnable() override {}
    void writeDisable() override {}
    bool isWriteEnabled() override { return false; }

    std::vector<std::string> getSupportedModels() const override { return supportedModels; }
    int resolveModelId(const std::string& modelStr) const override {
        for (size_t i = 0; i < supportedModels.size(); ++i) {
            if (supportedModels[i] == modelStr) return static_cast<int>(i);
        }
        return -1;
    }
};
