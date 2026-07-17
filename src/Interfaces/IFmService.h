#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

class IFmService {
public:
    virtual ~IFmService() = default;

    virtual bool configure(int8_t resetPin, int8_t sdaPin, int8_t sclPin, uint32_t i2cFreqHz = 100000) = 0;
    virtual bool isInitialized() const = 0;
    virtual bool isRunning() const = 0;
    virtual bool begin() = 0;
    virtual void stop() = 0;
    virtual void reset(uint8_t pin) = 0;
    virtual bool setTxPower(uint8_t dbuV, uint8_t antCap = 0) = 0;
    virtual uint8_t getTxPower() const = 0;
    virtual bool tune(uint16_t freq10kHz) = 0;
    virtual uint16_t getFrequency() const = 0;
    virtual bool beginRds() = 0;
    virtual bool setRdsStation(const char* ps8) = 0;
    virtual bool setRdsText(const char* radiotext) = 0;
    virtual bool setTrafficAnnouncement(bool enabled) = 0;
    virtual bool isTaEnabled() const = 0;
    virtual bool isRdsEnabled() const = 0;
    virtual bool measureAt(uint16_t freq10kHz, uint8_t& noiseLevel) = 0;
    virtual uint16_t scanBestFrequency(uint16_t start10kHz = 8750,
                                       uint16_t end10kHz = 10800,
                                       uint16_t step10kHz = 10) = 0;
    virtual size_t sweepActivity(std::vector<uint16_t>& freqs,
                                 std::vector<uint8_t>& levels,
                                 uint16_t start10kHz = 8750,
                                 uint16_t end10kHz = 10800,
                                 uint16_t step10kHz = 10,
                                 uint8_t samplesPerFreq = 1,
                                 uint16_t settleMs = 2) = 0;
};
