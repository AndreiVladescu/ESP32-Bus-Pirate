#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <vector>

#include "Interfaces/IFmService.h"

class FakeFmService final : public IFmService {
public:
    struct Configuration {
        int8_t resetPin = 0;
        int8_t sdaPin = 0;
        int8_t sclPin = 0;
        uint32_t i2cFreqHz = 0;
    };

    std::vector<Configuration> configurations;
    std::vector<uint8_t> resetPins;
    std::vector<uint16_t> measuredFrequencies;
    std::deque<uint8_t> noiseResults;

    bool configureResult = true;
    bool beginResult = true;
    bool initialized = false;
    bool running = false;
    bool measureResult = true;
    bool tuneResult = true;
    bool txPowerResult = true;
    bool rdsResult = true;
    bool taEnabled = false;
    bool rdsEnabled = false;
    uint8_t txPower = 0;
    uint16_t frequency = 0;
    uint32_t beginCalls = 0;
    uint32_t stopCalls = 0;
    uint32_t setTxPowerCalls = 0;
    uint32_t tuneCalls = 0;
    uint32_t beginRdsCalls = 0;
    uint32_t setRdsStationCalls = 0;
    uint32_t setRdsTextCalls = 0;
    uint32_t setTrafficAnnouncementCalls = 0;
    uint32_t scanBestFrequencyCalls = 0;
    uint32_t sweepActivityCalls = 0;

    bool configure(int8_t resetPin, int8_t sdaPin, int8_t sclPin,
                   uint32_t i2cFreqHz = 100000) override {
        configurations.push_back({resetPin, sdaPin, sclPin, i2cFreqHz});
        initialized = configureResult;
        return configureResult;
    }

    bool isInitialized() const override { return initialized; }
    bool isRunning() const override { return running; }

    bool begin() override {
        ++beginCalls;
        return beginResult;
    }

    void stop() override {
        ++stopCalls;
        running = false;
    }

    void reset(uint8_t pin) override {
        resetPins.push_back(pin);
        initialized = false;
    }

    bool setTxPower(uint8_t dbuV, uint8_t = 0) override {
        ++setTxPowerCalls;
        txPower = dbuV;
        return txPowerResult;
    }

    uint8_t getTxPower() const override { return txPower; }

    bool tune(uint16_t freq10kHz) override {
        ++tuneCalls;
        frequency = freq10kHz;
        running = tuneResult;
        return tuneResult;
    }

    uint16_t getFrequency() const override { return frequency; }

    bool beginRds() override {
        ++beginRdsCalls;
        rdsEnabled = rdsResult;
        return rdsResult;
    }

    bool setRdsStation(const char*) override {
        ++setRdsStationCalls;
        return rdsResult;
    }

    bool setRdsText(const char*) override {
        ++setRdsTextCalls;
        return rdsResult;
    }

    bool setTrafficAnnouncement(bool enabled) override {
        ++setTrafficAnnouncementCalls;
        taEnabled = enabled;
        return true;
    }

    bool isTaEnabled() const override { return taEnabled; }
    bool isRdsEnabled() const override { return rdsEnabled; }

    bool measureAt(uint16_t freq10kHz, uint8_t& noiseLevel) override {
        measuredFrequencies.push_back(freq10kHz);
        if (!measureResult) return false;
        if (!noiseResults.empty()) {
            noiseLevel = noiseResults.front();
            noiseResults.pop_front();
        } else {
            noiseLevel = 0;
        }
        return true;
    }

    uint16_t scanBestFrequency(uint16_t = 8750,
                               uint16_t = 10800,
                               uint16_t = 10) override {
        ++scanBestFrequencyCalls;
        return 0;
    }

    size_t sweepActivity(std::vector<uint16_t>&,
                         std::vector<uint8_t>&,
                         uint16_t = 8750,
                         uint16_t = 10800,
                         uint16_t = 10,
                         uint8_t = 1,
                         uint16_t = 2) override {
        ++sweepActivityCalls;
        return 0;
    }
};
