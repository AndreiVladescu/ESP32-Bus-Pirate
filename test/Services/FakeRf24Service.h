#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <string>
#include <vector>

#include "Interfaces/IRf24Service.h"

class FakeRf24Service final : public IRf24Service {
public:
    struct ConfigureCall {
        uint8_t csnPin = 0;
        uint8_t cePin = 0;
        uint8_t sckPin = 0;
        uint8_t misoPin = 0;
        uint8_t mosiPin = 0;
        uint32_t spiSpeed = 0;
    };

    struct SentPayload {
        std::vector<uint8_t> bytes;
    };

    bool configureResult = true;
    bool initRxResult = true;
    bool initTxResult = true;
    bool sendResult = true;
    bool carrier = false;
    bool rpd = false;
    bool connected = true;
    uint8_t channel = 76;
    uint8_t availablePipeValue = 0;
    std::deque<std::vector<uint8_t>> receivedPayloads;
    std::deque<bool> availablePipeValues;
    std::vector<ConfigureCall> configureCalls;
    std::vector<Rf24Config> initRxConfigs;
    std::vector<Rf24Config> initTxConfigs;
    std::vector<uint8_t> setChannels;
    std::vector<SentPayload> sentPayloads;
    uint32_t initRxSimpleCalls = 0;
    uint32_t startListeningCalls = 0;
    uint32_t stopListeningCalls = 0;
    uint32_t flushTxCalls = 0;
    uint32_t flushRxCalls = 0;
    uint32_t powerUpCalls = 0;
    uint32_t powerDownCalls = 0;
    uint32_t setPowerMaxCalls = 0;
    std::vector<rf24_datarate_e> dataRates;
    std::vector<rf24_crclength_e> crcLengths;
    std::vector<rf24_pa_dbm_e> powerLevels;

    bool configure(uint8_t csnPin,
                   uint8_t cePin,
                   uint8_t sckPin,
                   uint8_t misoPin,
                   uint8_t mosiPin,
                   SPIClass&,
                   uint32_t spiSpeed = 10000000) override {
        configureCalls.push_back({csnPin, cePin, sckPin, misoPin, mosiPin, spiSpeed});
        return configureResult;
    }

    void initRx() override { ++initRxSimpleCalls; }

    bool initRx(const Rf24Config& cfg) override {
        initRxConfigs.push_back(cfg);
        return initRxResult;
    }

    bool initTx(const Rf24Config& cfg) override {
        initTxConfigs.push_back(cfg);
        return initTxResult;
    }

    int getRxPayloadLen() override {
        return receivedPayloads.empty() ? 0 : static_cast<int>(receivedPayloads.front().size());
    }

    void setChannel(uint8_t value) override {
        channel = value;
        setChannels.push_back(value);
    }

    uint8_t getChannel() override { return channel; }
    void setDataRate(rf24_datarate_e rate) override { dataRates.push_back(rate); }
    void setCrcLength(rf24_crclength_e length) override { crcLengths.push_back(length); }
    void powerUp() override { ++powerUpCalls; }
    void powerDown(bool = false) override { ++powerDownCalls; }
    void setPowerLevel(rf24_pa_dbm_e level) override { powerLevels.push_back(level); }
    void setPowerMax() override { ++setPowerMaxCalls; }
    void openWritingPipe(const uint64_t) override {}
    void openReadingPipe(uint8_t, const uint64_t) override {}
    void startListening() override { ++startListeningCalls; }
    void stopListening() override { ++stopListeningCalls; }

    bool send(const void* buf, uint8_t len) override {
        const auto* bytes = static_cast<const uint8_t*>(buf);
        sentPayloads.push_back({std::vector<uint8_t>(bytes, bytes + len)});
        return sendResult;
    }

    bool receive(void* buf, uint8_t len) override {
        if (receivedPayloads.empty()) return false;
        auto payload = receivedPayloads.front();
        receivedPayloads.pop_front();
        const size_t count = std::min<size_t>(len, payload.size());
        std::memcpy(buf, payload.data(), count);
        return true;
    }

    bool receive(uint8_t* out, size_t outMax, uint8_t& outLen) override {
        if (receivedPayloads.empty()) return false;
        auto payload = receivedPayloads.front();
        receivedPayloads.pop_front();
        const size_t count = std::min(outMax, payload.size());
        std::memcpy(out, payload.data(), count);
        outLen = static_cast<uint8_t>(count);
        return true;
    }

    bool availablePipe(uint8_t* pipe = nullptr) override {
        bool available = !receivedPayloads.empty();
        if (!availablePipeValues.empty()) {
            available = availablePipeValues.front();
            availablePipeValues.pop_front();
        }
        if (available && pipe != nullptr) *pipe = availablePipeValue;
        return available;
    }

    bool available() override { return !receivedPayloads.empty(); }
    bool isChipConnected() override { return connected; }
    void flushTx() override { ++flushTxCalls; }
    void flushRx() override { ++flushRxCalls; }
    bool testCarrier() override { return carrier; }
    bool testRpd() override { return rpd; }
};
