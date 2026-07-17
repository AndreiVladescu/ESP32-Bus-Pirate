#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "Interfaces/ISpiService.h"

class FakeSpiService final : public ISpiService {
public:
    struct Configuration {
        uint8_t mosi = 0;
        uint8_t miso = 0;
        uint8_t sclk = 0;
        uint8_t cs = 0;
        uint32_t frequency = 0;
    };

    struct SlaveCall {
        int sclk = 0;
        int miso = 0;
        int mosi = 0;
        int cs = 0;
    };

    std::vector<Configuration> configurations;
    std::vector<SlaveCall> startSlaveCalls;
    std::vector<SlaveCall> stopSlaveCalls;
    std::vector<std::vector<std::vector<uint8_t>>> slaveDataBatches;
    std::vector<ByteCode> lastBytecodes;
    std::string byteCodeResult;
    uint32_t endCalls = 0;
    uint32_t beginTransactionCalls = 0;
    uint32_t endTransactionCalls = 0;
    std::vector<uint8_t> transfers;
    bool slave = false;

    void configure(uint8_t mosi, uint8_t miso, uint8_t sclk, uint8_t cs, uint32_t frequency = 1000000) override {
        configurations.push_back({mosi, miso, sclk, cs, frequency});
    }

    void end() override {
        ++endCalls;
        slave = false;
    }

    void beginTransaction() override { ++beginTransactionCalls; }
    void endTransaction() override { ++endTransactionCalls; }

    uint8_t transfer(uint8_t data) override {
        transfers.push_back(data);
        return data;
    }

    std::string readFlashID() override { return ""; }
    void readFlashIdRaw(uint8_t* buffer) override {
        if (buffer == nullptr) return;
        buffer[0] = 0;
        buffer[1] = 0;
        buffer[2] = 0;
    }
    void readFlashData(uint32_t, uint8_t* buffer, size_t length) override {
        if (buffer == nullptr) return;
        for (size_t i = 0; i < length; ++i) buffer[i] = 0;
    }
    uint32_t calculateFlashCapacity(uint8_t) override { return 0; }
    void eraseFlashSector(uint32_t, uint32_t) override {}
    void enableFlashWrite(uint32_t) override {}
    void waitForFlashWriteComplete(uint32_t) override {}
    void writeFlashPage(uint32_t, const std::vector<uint8_t>&, uint32_t) override {}
    void writeFlashPatch(uint32_t, const std::vector<uint8_t>&, uint32_t) override {}

    bool initEeprom(uint8_t, uint8_t, uint8_t, uint8_t, uint16_t, uint32_t, uint16_t = 255, bool = false) override {
        return true;
    }
    bool probeEeprom() override { return true; }
    bool writeEeprom(uint32_t, uint8_t) override { return true; }
    uint8_t readEeprom(uint32_t) override { return 0; }
    bool writeEepromBuffer(uint32_t, const uint8_t*, size_t) override { return true; }
    bool readEepromBuffer(uint32_t, uint8_t*, size_t) override { return true; }
    bool writeEepromInt(uint32_t, int32_t) override { return true; }
    int32_t readEepromInt(uint32_t) override { return 0; }
    bool writeEepromFloat(uint32_t, float) override { return true; }
    float readEepromFloat(uint32_t) override { return 0.0f; }
    bool writeEepromString(uint32_t, const std::string&) override { return true; }
    bool readEepromString(uint32_t, std::string& str) override {
        str.clear();
        return true;
    }
    void eraseEepromChip() override {}
    void eraseEepromSector(uint32_t) override {}
    void eraseEepromPage(uint32_t) override {}
    void closeEeprom() override {}

    void startSlave(int sclk, int miso, int mosi, int cs) override {
        startSlaveCalls.push_back({sclk, miso, mosi, cs});
        slave = true;
    }

    void stopSlave(int sclk, int miso, int mosi, int cs) override {
        stopSlaveCalls.push_back({sclk, miso, mosi, cs});
        slave = false;
    }

    bool isSlave() const override { return slave; }

    std::vector<std::vector<uint8_t>> getSlaveData() override {
        if (slaveDataBatches.empty()) return {};
        auto batch = slaveDataBatches.front();
        slaveDataBatches.erase(slaveDataBatches.begin());
        return batch;
    }

    std::string executeByteCode(const std::vector<ByteCode>& bytecodes) override {
        lastBytecodes = bytecodes;
        return byteCodeResult;
    }
};
