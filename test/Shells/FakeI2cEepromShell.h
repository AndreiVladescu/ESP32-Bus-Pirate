#pragma once

#include <cstdint>
#include <vector>

#include "Interfaces/II2cEepromShell.h"

class FakeI2cEepromShell final : public II2cEepromShell {
public:
    std::vector<uint8_t> addresses;

    void run(uint8_t address = 0x50) override { addresses.push_back(address); }
};
