#pragma once

#include <deque>
#include <string>
#include <vector>

#include "Interfaces/ICanService.h"

class FakeCanService final : public ICanService {
public:
    struct Configuration {
        uint8_t csPin = 0;
        uint8_t sckPin = 0;
        uint8_t misoPin = 0;
        uint8_t mosiPin = 0;
        uint32_t bitrateKbps = 0;
    };

    struct SentFrame {
        uint32_t id = 0;
        std::vector<uint8_t> data;
    };

    Configuration lastConfiguration;
    std::vector<SentFrame> sentFrames;
    std::deque<std::string> receivedFrames;
    std::string status = "CAN ready";
    uint32_t supportedBitrate = 125;
    uint32_t requestedBitrate = 0;
    uint32_t lastFilter = 0;
    bool sendResult = true;
    bool probeResult = true;
    uint32_t configureCalls = 0;
    uint32_t resetCalls = 0;
    uint32_t flushCalls = 0;
    uint32_t setFilterCalls = 0;
    uint32_t probeCalls = 0;

    void configure(uint8_t csPin,
                   uint8_t sck,
                   uint8_t miso,
                   uint8_t mosi,
                   uint32_t bitrateKbps) override {
        lastConfiguration = {csPin, sck, miso, mosi, bitrateKbps};
        ++configureCalls;
    }

    void reset() override { ++resetCalls; }
    void flush() override { ++flushCalls; }

    bool sendFrame(uint32_t id, const std::vector<uint8_t>& data) override {
        sentFrames.push_back({id, data});
        return sendResult;
    }

    std::string readFrameAsString() override {
        if (receivedFrames.empty()) return "";
        std::string frame = receivedFrames.front();
        receivedFrames.pop_front();
        return frame;
    }

    uint32_t closestSupportedBitrate(uint32_t kbps) override {
        requestedBitrate = kbps;
        return supportedBitrate;
    }

    void setFilter(uint32_t id) override {
        lastFilter = id;
        ++setFilterCalls;
    }
    std::string getStatus() override { return status; }
    bool probe() override {
        ++probeCalls;
        return probeResult;
    }
};
