#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Si4713.h>
#include "Interfaces/IFmService.h"

class FmService : public IFmService {
public:
    FmService() = default;
    ~FmService();

    bool configure(int8_t resetPin, int8_t sdaPin, int8_t sclPin, uint32_t i2cFreqHz = 100000);
    bool isInitialized() const { return isInitialized_; }
    bool isRunning() const { return isRunning_; }
    bool begin();          // radio.begin()
    void stop();           // power down / reset / tx power 0
    void reset(uint8_t pin);          // hard reset via reset pin 
    
    // TX parameters
    bool setTxPower(uint8_t dbuV, uint8_t antCap = 0);  // 88..115, 0..15
    uint8_t getTxPower() const { return txPower_; }
    bool tune(uint16_t freq10kHz);  // ex: 10230  (102.30MHz)
    uint16_t getFrequency() const { return frequency10kHz_; }

    // RDS / TA
    bool beginRds();
    bool setRdsStation(const char* ps8);     // 8 chars max
    bool setRdsText(const char* radiotext);  // 64 chars max
    bool setTrafficAnnouncement(bool enabled);
    bool isTaEnabled() const { return isTaEnabled_; };
    bool isRdsEnabled() const { return isRdsEnabled_; };

    // Mesures frequencies noise level 
    bool measureAt(uint16_t freq10kHz, uint8_t& noiseLevel);
    uint16_t scanBestFrequency(uint16_t start10kHz = 8750,
                               uint16_t end10kHz   = 10800,
                               uint16_t step10kHz  = 10);

    size_t sweepActivity(std::vector<uint16_t>& freqs,
                        std::vector<uint8_t>& levels,
                        uint16_t start10kHz = 8750,
                        uint16_t end10kHz   = 10800,
                        uint16_t step10kHz  = 10,
                        uint8_t samplesPerFreq = 1,
                        uint16_t settleMs = 2);

private:
    Adafruit_Si4713* radio_ = nullptr;

    bool isInitialized_ = false;
    bool isRunning_ = false;
    bool isRdsEnabled_ = false;
    bool isTaEnabled_ = false;

    uint16_t frequency10kHz_ = 0;
    uint8_t txPower_ = 0;
};
