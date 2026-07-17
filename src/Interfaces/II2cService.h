#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "Models/ByteCode.h"

struct I2cRegProbeResult {
    bool readable = false;
    bool writeOk = false;
    bool readback = false;
    bool rwHit = false;

    uint8_t oldVal = 0;
    uint8_t newVal = 0;
};

class II2cService {
public:
    virtual ~II2cService() = default;

    virtual void configure(uint8_t sda, uint8_t scl, uint32_t frequency = 100000) = 0;
    virtual void beginTransmission(uint8_t address) = 0;
    virtual void write(uint8_t data) = 0;
    virtual bool endTransmission(bool sendStop = true) = 0;
    virtual uint8_t requestFrom(uint8_t address, uint8_t quantity, bool sendStop = true) = 0;
    virtual int read() = 0;
    virtual bool available() const = 0;
    virtual bool end() const = 0;
    virtual bool isReadableDevice(uint8_t addr, uint8_t startReg) = 0;
    virtual bool ping(uint8_t addr, bool sendStop = true, uint32_t* outDtUs = nullptr) = 0;
    virtual bool readReg(uint8_t addr, uint8_t reg, uint8_t* outVal, uint32_t* outDtUs = nullptr) = 0;
    virtual bool writeReg(uint8_t addr, uint8_t reg, uint8_t val) = 0;
    virtual bool probeRegRW(uint8_t addr, uint8_t reg, I2cRegProbeResult& out) = 0;

    virtual void i2cBitBangDelay(uint32_t delayUs) = 0;
    virtual void i2cBitBangSetLevel(uint8_t pin, bool level) = 0;
    virtual void i2cBitBangSetOutput(uint8_t pin) = 0;
    virtual void i2cBitBangSetInput(uint8_t pin) = 0;
    virtual void i2cBitBangWriteBit(uint8_t scl, uint8_t sda, bool bit, uint32_t delayUs) = 0;
    virtual void i2cBitBangWriteByte(uint8_t scl, uint8_t sda, uint8_t data, uint32_t delayUs, bool& ack) = 0;
    virtual void i2cBitBangReadByte(uint8_t scl, uint8_t sda, uint32_t delayUs, bool nackLast) = 0;
    virtual void i2cBitBangStartCondition(uint8_t scl, uint8_t sda, uint32_t delayUs) = 0;
    virtual void i2cBitBangStopCondition(uint8_t scl, uint8_t sda, uint32_t delayUs) = 0;
    virtual bool i2cBitBangRecoverBus(uint8_t scl, uint8_t sda, uint32_t freqHz) = 0;

    static constexpr size_t SLAVE_LOG_MAX = 128;
    virtual void beginSlave(uint8_t address, uint8_t sda, uint8_t scl, uint32_t freq = 100000) = 0;
    virtual void endSlave() = 0;
    virtual std::vector<std::string> getSlaveLog() = 0;
    virtual uint32_t getSlaveLogCount() = 0;
    virtual void clearSlaveLog() = 0;

    virtual void rapidStartStop(uint8_t address, uint32_t freqHz, uint8_t sclPin, uint8_t sdaPin) = 0;
    virtual void floodRandom(uint8_t address, uint32_t freqHz, uint8_t sclPin, uint8_t sdaPin) = 0;
    virtual void floodStart(uint8_t address, uint32_t freqHz, uint8_t scl, uint8_t sda) = 0;
    virtual void overReadAttack(uint8_t address, uint32_t freqHz, uint8_t sclPin, uint8_t sdaPin) = 0;
    virtual void invalidRegisterRead(uint8_t address, uint32_t freqHz, uint8_t sclPin, uint8_t sdaPin) = 0;
    virtual void simulateClockStretch(uint8_t address, uint32_t freqHz, uint8_t sclPin, uint8_t sdaPin) = 0;
    virtual void sclSdaGlitch(uint8_t sclPin, uint8_t sdaPin) = 0;
    virtual void randomClockPulseNoise(uint8_t sclPin, uint8_t sdaPin, uint32_t freqHz) = 0;
    virtual void glitchAckInjection(uint8_t address, uint32_t freqHz, uint8_t sclPin, uint8_t sdaPin) = 0;
    virtual void injectRandomGlitch(uint8_t sclPin, uint8_t sdaPin, uint32_t freqHz) = 0;

    virtual std::string executeByteCode(const std::vector<ByteCode>& bytecodes) = 0;

    virtual bool initEeprom(uint16_t chipSizeKb = 512, uint8_t addr = 0x50) = 0;
    virtual bool eepromWriteByte(uint32_t address, uint8_t value) = 0;
    virtual uint8_t eepromReadByte(uint32_t address) = 0;
    virtual bool eepromPutString(uint32_t address, const std::string& str) = 0;
    virtual bool eepromGetString(uint32_t address, std::string& outStr) = 0;
    virtual uint32_t eepromLength() = 0;
    virtual uint32_t eepromGetMemorySize() = 0;
    virtual uint16_t eepromPageSize() = 0;
    virtual uint8_t eepromWriteTimeMs() = 0;
    virtual uint8_t eepromAddressBytes() = 0;
    virtual bool eepromIsConnected() = 0;
    virtual bool eepromIsBusy() = 0;
    virtual void eepromErase(uint8_t fill = 0xFF) = 0;
    virtual bool eepromDetectMemorySize() = 0;
    virtual uint8_t eepromDetectAddressBytes() = 0;
    virtual uint16_t eepromDetectPageSize() = 0;
    virtual uint8_t eepromDetectWriteTime(uint8_t testCount = 8) = 0;

    virtual bool tryPowerOnSticks3Pmic(uint32_t timeout) = 0;
};
