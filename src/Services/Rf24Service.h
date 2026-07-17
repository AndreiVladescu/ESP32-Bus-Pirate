#pragma once

#include <Arduino.h>
#include <RF24.h>
#include <vector>
#include <string>
#include "Interfaces/IRf24Service.h"

class Rf24Service : public IRf24Service {
public:
    // Configuration
    bool configure(
        uint8_t csnPin,
        uint8_t cePin,
        uint8_t sckPin,
        uint8_t misoPin,
        uint8_t mosiPin,
        SPIClass& spi,
        uint32_t spiSpeed = 10000000
    ) override;

    void initRx() override;
    bool initRx(const Rf24Config& cfg) override;
    bool initTx(const Rf24Config& cfg) override;

    // Base
    int getRxPayloadLen() override;
    void setChannel(uint8_t channel) override;              // 0..125
    uint8_t getChannel() override;
    void setDataRate(rf24_datarate_e rate) override;        // RF24_250KBPS, RF24_1MBPS, RF24_2MBPS
    void setCrcLength(rf24_crclength_e length) override;    // RF24_CRC_DISABLED, RF24_CRC_8, RF24_CRC_16
    void powerUp() override;
    void powerDown(bool hard = false) override;
    void setPowerLevel(rf24_pa_dbm_e level) override;       // RF24_PA_MIN .. RF24_PA_MAX
    void setPowerMax() override;

    // Communication
    void openWritingPipe(const uint64_t address) override;
    void openReadingPipe(uint8_t number, const uint64_t address) override;
    void startListening() override;
    void stopListening() override;

    // IO
    bool send(const void* buf, uint8_t len) override;
    bool receive(void* buf, uint8_t len) override;
    bool receive(uint8_t* out, size_t outMax, uint8_t& outLen) override;
    bool availablePipe(uint8_t* pipe = nullptr) override;
    bool available() override;

    // Utils
    bool isChipConnected() override;
    void flushTx() override;
    void flushRx() override;
    bool testCarrier() override;
    bool testRpd() override;

private:
    void initTembedPlus();
    RF24* radio_ = nullptr;
    bool isInitialized = false;
    uint8_t cePin_ = 0;
    uint8_t csnPin_ = 0;
    uint8_t sckPin_ = 0;
    uint8_t misoPin_ = 0;
    uint8_t mosiPin_ = 0;
    uint32_t spiSpeed_ = 10000000;
    bool dynamicPayloadEnabled = false;
};
