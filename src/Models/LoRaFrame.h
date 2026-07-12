#pragma once

#include <cstdint>
#include <vector>

#include "Models/LoRaRadioProfile.h"

struct LoRaFrame {
    std::vector<uint8_t> payload;
    LoRaRadioProfile profile;
    float rssi = 0.0f;
    float snr = 0.0f;
};
