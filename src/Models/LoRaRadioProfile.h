#pragma once

#include <cstdint>

struct LoRaRadioProfile {
    float frequency = 868.0f;
    uint16_t bandwidth = 125;
    uint8_t spreadingFactor = 9;
    uint8_t codingRate = 7;
    int8_t power = 14;
    float tcxoVoltage = 1.8f;
    uint16_t preambleLength = 8;
    uint16_t syncWord = 0x1424;
    bool crc = true;
    bool invertIq = false;
};
