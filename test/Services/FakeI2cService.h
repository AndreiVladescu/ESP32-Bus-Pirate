#pragma once

#include <algorithm>
#include <cstdint>
#include <deque>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "Interfaces/II2cService.h"

class FakeI2cService final : public II2cService {
public:
    struct Configuration {
        uint8_t sda = 0;
        uint8_t scl = 0;
        uint32_t frequency = 0;
    };

    struct Request {
        uint8_t address = 0;
        uint8_t quantity = 0;
        bool sendStop = true;
    };

    std::vector<Configuration> configurations;
    std::vector<uint8_t> beginAddresses;
    std::vector<uint8_t> writtenBytes;
    std::vector<bool> endTransmissionStops;
    std::vector<Request> requests;
    std::deque<bool> endTransmissionResults;
    std::deque<uint8_t> requestResults;
    std::deque<int> rxData;
    std::set<uint8_t> acknowledgedAddresses;
    bool useAddressAcknowledgements = false;
    uint8_t currentAddress = 0;
    mutable uint32_t endCalls = 0;
    uint32_t endTransmissionCalls = 0;
    std::string byteCodeResult;
    std::vector<ByteCode> lastBytecodes;

    bool readableDevice = false;
    bool pingResult = false;
    uint32_t pingDtUs = 10;
    uint32_t pingCalls = 0;
    uint8_t lastPingAddress = 0;
    bool readRegResult = false;
    uint8_t readRegValue = 0;
    uint32_t readRegDtUs = 10;
    uint32_t readRegCalls = 0;
    std::deque<std::pair<bool, uint8_t>> readRegResults;
    bool writeRegResult = false;
    bool probeRegResult = false;
    I2cRegProbeResult probeResult;

    bool recoverResult = false;
    uint32_t recoverCalls = 0;
    uint8_t recoverScl = 0;
    uint8_t recoverSda = 0;
    uint32_t recoverFrequency = 0;

    uint32_t clearSlaveLogCalls = 0;
    uint32_t beginSlaveCalls = 0;
    uint32_t endSlaveCalls = 0;
    uint8_t slaveAddress = 0;
    uint8_t slaveSda = 0;
    uint8_t slaveScl = 0;
    uint32_t slaveFrequency = 0;
    std::vector<std::string> slaveLog;
    uint32_t slaveLogCount = 0;

    uint32_t rapidStartStopCalls = 0;
    uint32_t floodRandomCalls = 0;
    uint32_t floodStartCalls = 0;
    uint32_t overReadAttackCalls = 0;
    uint32_t invalidRegisterReadCalls = 0;
    uint32_t simulateClockStretchCalls = 0;
    uint32_t sclSdaGlitchCalls = 0;
    uint32_t randomClockPulseNoiseCalls = 0;
    uint32_t glitchAckInjectionCalls = 0;
    uint32_t injectRandomGlitchCalls = 0;

    void configure(uint8_t sda, uint8_t scl, uint32_t frequency = 100000) override {
        configurations.push_back({sda, scl, frequency});
    }

    void beginTransmission(uint8_t address) override {
        currentAddress = address;
        beginAddresses.push_back(address);
    }

    void write(uint8_t data) override { writtenBytes.push_back(data); }

    bool endTransmission(bool sendStop = true) override {
        ++endTransmissionCalls;
        endTransmissionStops.push_back(sendStop);
        if (!endTransmissionResults.empty()) {
            const bool result = endTransmissionResults.front();
            endTransmissionResults.pop_front();
            return result;
        }
        if (useAddressAcknowledgements) {
            return acknowledgedAddresses.find(currentAddress) == acknowledgedAddresses.end();
        }
        return false;
    }

    uint8_t requestFrom(uint8_t address, uint8_t quantity, bool sendStop = true) override {
        requests.push_back({address, quantity, sendStop});
        if (!requestResults.empty()) {
            const uint8_t result = requestResults.front();
            requestResults.pop_front();
            return result;
        }
        return static_cast<uint8_t>(std::min<size_t>(quantity, rxData.size()));
    }

    int read() override {
        if (rxData.empty()) return -1;
        const int result = rxData.front();
        rxData.pop_front();
        return result;
    }

    bool available() const override { return !rxData.empty(); }

    bool end() const override {
        ++endCalls;
        return true;
    }

    bool isReadableDevice(uint8_t, uint8_t) override { return readableDevice; }

    bool ping(uint8_t addr, bool = true, uint32_t* outDtUs = nullptr) override {
        ++pingCalls;
        lastPingAddress = addr;
        if (outDtUs != nullptr) *outDtUs = pingDtUs;
        return pingResult;
    }

    bool readReg(uint8_t, uint8_t, uint8_t* outVal, uint32_t* outDtUs = nullptr) override {
        ++readRegCalls;
        bool result = readRegResult;
        uint8_t value = readRegValue;
        if (!readRegResults.empty()) {
            result = readRegResults.front().first;
            value = readRegResults.front().second;
            readRegResults.pop_front();
        }
        if (outVal != nullptr) *outVal = value;
        if (outDtUs != nullptr) *outDtUs = readRegDtUs;
        return result;
    }

    bool writeReg(uint8_t, uint8_t, uint8_t) override { return writeRegResult; }

    bool probeRegRW(uint8_t, uint8_t, I2cRegProbeResult& out) override {
        out = probeResult;
        return probeRegResult;
    }

    void i2cBitBangDelay(uint32_t) override {}
    void i2cBitBangSetLevel(uint8_t, bool) override {}
    void i2cBitBangSetOutput(uint8_t) override {}
    void i2cBitBangSetInput(uint8_t) override {}
    void i2cBitBangWriteBit(uint8_t, uint8_t, bool, uint32_t) override {}
    void i2cBitBangWriteByte(uint8_t, uint8_t, uint8_t, uint32_t, bool& ack) override { ack = true; }
    void i2cBitBangReadByte(uint8_t, uint8_t, uint32_t, bool) override {}
    void i2cBitBangStartCondition(uint8_t, uint8_t, uint32_t) override {}
    void i2cBitBangStopCondition(uint8_t, uint8_t, uint32_t) override {}

    bool i2cBitBangRecoverBus(uint8_t scl, uint8_t sda, uint32_t freqHz) override {
        ++recoverCalls;
        recoverScl = scl;
        recoverSda = sda;
        recoverFrequency = freqHz;
        return recoverResult;
    }

    void beginSlave(uint8_t address, uint8_t sda, uint8_t scl,
                    uint32_t freq = 100000) override {
        ++beginSlaveCalls;
        slaveAddress = address;
        slaveSda = sda;
        slaveScl = scl;
        slaveFrequency = freq;
    }

    void endSlave() override { ++endSlaveCalls; }
    std::vector<std::string> getSlaveLog() override { return slaveLog; }
    uint32_t getSlaveLogCount() override { return slaveLogCount; }
    void clearSlaveLog() override { ++clearSlaveLogCalls; }

    void rapidStartStop(uint8_t, uint32_t, uint8_t, uint8_t) override { ++rapidStartStopCalls; }
    void floodRandom(uint8_t, uint32_t, uint8_t, uint8_t) override { ++floodRandomCalls; }
    void floodStart(uint8_t, uint32_t, uint8_t, uint8_t) override { ++floodStartCalls; }
    void overReadAttack(uint8_t, uint32_t, uint8_t, uint8_t) override { ++overReadAttackCalls; }
    void invalidRegisterRead(uint8_t, uint32_t, uint8_t, uint8_t) override { ++invalidRegisterReadCalls; }
    void simulateClockStretch(uint8_t, uint32_t, uint8_t, uint8_t) override { ++simulateClockStretchCalls; }
    void sclSdaGlitch(uint8_t, uint8_t) override { ++sclSdaGlitchCalls; }
    void randomClockPulseNoise(uint8_t, uint8_t, uint32_t) override { ++randomClockPulseNoiseCalls; }
    void glitchAckInjection(uint8_t, uint32_t, uint8_t, uint8_t) override { ++glitchAckInjectionCalls; }
    void injectRandomGlitch(uint8_t, uint8_t, uint32_t) override { ++injectRandomGlitchCalls; }

    std::string executeByteCode(const std::vector<ByteCode>& bytecodes) override {
        lastBytecodes = bytecodes;
        return byteCodeResult;
    }

    bool initEeprom(uint16_t = 512, uint8_t = 0x50) override { return false; }
    bool eepromWriteByte(uint32_t, uint8_t) override { return false; }
    uint8_t eepromReadByte(uint32_t) override { return 0; }
    bool eepromPutString(uint32_t, const std::string&) override { return false; }
    bool eepromGetString(uint32_t, std::string&) override { return false; }
    uint32_t eepromLength() override { return 0; }
    uint32_t eepromGetMemorySize() override { return 0; }
    uint16_t eepromPageSize() override { return 0; }
    uint8_t eepromWriteTimeMs() override { return 0; }
    uint8_t eepromAddressBytes() override { return 0; }
    bool eepromIsConnected() override { return false; }
    bool eepromIsBusy() override { return false; }
    void eepromErase(uint8_t = 0xFF) override {}
    bool eepromDetectMemorySize() override { return false; }
    uint8_t eepromDetectAddressBytes() override { return 0; }
    uint16_t eepromDetectPageSize() override { return 0; }
    uint8_t eepromDetectWriteTime(uint8_t = 8) override { return 0; }
    bool tryPowerOnSticks3Pmic(uint32_t) override { return false; }
};
