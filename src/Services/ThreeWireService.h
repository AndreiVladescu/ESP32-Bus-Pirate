#pragma once

#include <Arduino.h>
#include <vector>
#include <string>
#include "Interfaces/IThreeWireService.h"

extern "C" {
    #include "93Cx6.h"
}

class ThreeWireService : public IThreeWireService {
public:
    void configure(uint8_t cs, uint8_t sk, uint8_t di, uint8_t doPin, int16_t model = 66, bool org8 = false) override;
    void end() override;

    uint16_t read16(uint16_t addr) override;
    uint8_t read8(uint16_t addr) override;
    void write16(uint16_t addr, uint16_t value) override;
    void write8(uint16_t addr, uint8_t value) override;
    void writeAll(uint16_t value) override;

    void erase(uint16_t addr) override;
    void eraseAll() override;

    std::vector<uint8_t> dump8() override;
    std::vector<uint16_t> dump16() override;
    uint16_t sizeBytes() const override;

    void writeEnable() override;
    void writeDisable() override;
    bool isWriteEnabled() override;

    std::vector<std::string> getSupportedModels() const override;
    int resolveModelId(const std::string& modelStr) const override;
private:
    EEPROM_T eeprom;
    uint16_t eepromSizeBytes = 0;
    int16_t eepromOrgMode = EEPROM_MODE_16BIT;
};
