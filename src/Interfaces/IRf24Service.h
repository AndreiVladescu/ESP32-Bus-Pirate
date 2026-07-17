#pragma once

#if __has_include("Vendors/Arduino.h")
#include "Vendors/Arduino.h"
#else
#include <Arduino.h>
#endif

#if __has_include("Vendors/FakeRf24.h")
#include "Vendors/FakeRf24.h"
#else
#define BUSPIRATE_RF24_HEADER <RF24.h>
#include BUSPIRATE_RF24_HEADER
#undef BUSPIRATE_RF24_HEADER
#endif
#include <string>

class SPIClass;

class IRf24Service {
public:
    struct Rf24Config {
        uint8_t channel = 76;
        uint8_t pipe = 1;
        uint8_t addr[5] = {0};
        std::string addrStr;
        uint8_t addrLen = 5;
        int crcBits = 16;
        int dataRate = 1;
        bool dynamicPayloads = true;
        uint8_t fixedPayloadSize = 32;
    };

    virtual ~IRf24Service() = default;

    virtual bool configure(
        uint8_t csnPin,
        uint8_t cePin,
        uint8_t sckPin,
        uint8_t misoPin,
        uint8_t mosiPin,
        SPIClass& spi,
        uint32_t spiSpeed = 10000000
    ) = 0;

    virtual void initRx() = 0;
    virtual bool initRx(const Rf24Config& cfg) = 0;
    virtual bool initTx(const Rf24Config& cfg) = 0;

    virtual int getRxPayloadLen() = 0;
    virtual void setChannel(uint8_t channel) = 0;
    virtual uint8_t getChannel() = 0;
    virtual void setDataRate(rf24_datarate_e rate) = 0;
    virtual void setCrcLength(rf24_crclength_e length) = 0;
    virtual void powerUp() = 0;
    virtual void powerDown(bool hard = false) = 0;
    virtual void setPowerLevel(rf24_pa_dbm_e level) = 0;
    virtual void setPowerMax() = 0;

    virtual void openWritingPipe(const uint64_t address) = 0;
    virtual void openReadingPipe(uint8_t number, const uint64_t address) = 0;
    virtual void startListening() = 0;
    virtual void stopListening() = 0;

    virtual bool send(const void* buf, uint8_t len) = 0;
    virtual bool receive(void* buf, uint8_t len) = 0;
    virtual bool receive(uint8_t* out, size_t outMax, uint8_t& outLen) = 0;
    virtual bool availablePipe(uint8_t* pipe = nullptr) = 0;
    virtual bool available() = 0;

    virtual bool isChipConnected() = 0;
    virtual void flushTx() = 0;
    virtual void flushRx() = 0;
    virtual bool testCarrier() = 0;
    virtual bool testRpd() = 0;
};
