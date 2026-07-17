#pragma once

#include <cstdint>
#include <string>
#include <vector>

class IThreeWireService {
public:
    virtual ~IThreeWireService() = default;

    virtual void configure(uint8_t cs, uint8_t sk, uint8_t di, uint8_t doPin,
                           int16_t model = 66, bool org8 = false) = 0;
    virtual void end() = 0;

    virtual uint16_t read16(uint16_t addr) = 0;
    virtual uint8_t read8(uint16_t addr) = 0;
    virtual void write16(uint16_t addr, uint16_t value) = 0;
    virtual void write8(uint16_t addr, uint8_t value) = 0;
    virtual void writeAll(uint16_t value) = 0;

    virtual void erase(uint16_t addr) = 0;
    virtual void eraseAll() = 0;

    virtual std::vector<uint8_t> dump8() = 0;
    virtual std::vector<uint16_t> dump16() = 0;
    virtual uint16_t sizeBytes() const = 0;

    virtual void writeEnable() = 0;
    virtual void writeDisable() = 0;
    virtual bool isWriteEnabled() = 0;

    virtual std::vector<std::string> getSupportedModels() const = 0;
    virtual int resolveModelId(const std::string& modelStr) const = 0;
};
