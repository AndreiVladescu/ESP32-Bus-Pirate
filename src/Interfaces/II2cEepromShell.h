#pragma once

#include <cstdint>

class II2cEepromShell {
public:
    virtual ~II2cEepromShell() = default;
    virtual void run(uint8_t address = 0x50) = 0;
};
