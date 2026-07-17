#pragma once

#include <cstdint>
#include <string>
#include <vector>

class ICanService {
public:
    virtual ~ICanService() = default;

    virtual void configure(uint8_t csPin,
                           uint8_t sck,
                           uint8_t miso,
                           uint8_t mosi,
                           uint32_t bitrateKbps) = 0;
    virtual void reset() = 0;
    virtual void flush() = 0;

    virtual bool sendFrame(uint32_t id, const std::vector<uint8_t>& data) = 0;
    virtual std::string readFrameAsString() = 0;

    virtual uint32_t closestSupportedBitrate(uint32_t kbps) = 0;
    virtual void setFilter(uint32_t id) = 0;
    virtual std::string getStatus() = 0;
    virtual bool probe() = 0;
};
