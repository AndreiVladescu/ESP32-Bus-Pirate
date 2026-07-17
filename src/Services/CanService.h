#pragma once

#include <string>
#include <vector>
#include <mcp2515.h>
#include <Arduino.h>
#include "Interfaces/ICanService.h"

// Must be global to work, cs pin needs to be set at compile time
#ifdef DEVICE_TEMBEDS3CC1101
    MCP2515 mcp2515 = MCP2515(CAN_CS_PIN, 10000000, &SPI); // avoid screen conflicts
#else 
    MCP2515 mcp2515 = MCP2515(CAN_CS_PIN);
#endif

class CanService : public ICanService {
public:
    void configure(uint8_t csPin, uint8_t sck, uint8_t miso, uint8_t mosi, uint32_t bitrateKbps = 125) override;
    void end();
    void reset() override;
    void flush() override;

    bool sendFrame(uint32_t id, const std::vector<uint8_t>& data) override;
    bool readFrame(struct can_frame& outFrame);
    std::string readFrameAsString() override;  // pour affichage

    void setBitrate(uint32_t bitrateKbps);
    uint32_t closestSupportedBitrate(uint32_t kbps) override;
    void setFilter(uint32_t id) override;
    void setMask(uint32_t mask);

    std::string getStatus() override;
    bool probe() override;
    
private:
    CAN_SPEED resolveBitrate(uint32_t kbps);
    uint8_t csPin, sckPin, misoPin, mosiPin;
    uint32_t kbps;

};
