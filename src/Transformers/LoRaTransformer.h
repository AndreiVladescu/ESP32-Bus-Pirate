#pragma once

#include <string>
#include <vector>

#include "Models/LoRaFrame.h"

class LoRaTransformer {
public:
    bool transform(const std::string& raw, std::vector<uint8_t>& payload) const;
    std::string transformToFileFormat(const LoRaFrame& frame) const;
    bool transformFromFileFormat(const std::string& text, LoRaFrame& frame) const;
    LoRaFrame fromCapture(const std::vector<uint8_t>& payload,
                          const LoRaRadioProfile& profile,
                          float rssi, float snr) const;
};
