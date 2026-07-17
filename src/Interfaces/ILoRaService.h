#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "Models/LoRaFrame.h"

class SPIClass;

class ILoRaService {
public:
    enum ReceiveResult : int16_t {
        RECEIVE_OK = 0,
        RECEIVE_NOT_INITIALIZED = -1,
        RECEIVE_TIMEOUT = 1,
        RECEIVE_ERROR = 2
    };

    struct RssiStats {
        int16_t minimum = 0;
        int16_t maximum = 0;
        float average = 0.0f;
        uint32_t samples = 0;
    };

    virtual ~ILoRaService() = default;

    virtual bool configure(SPIClass& spi, uint8_t sck, uint8_t miso, uint8_t mosi,
                           uint8_t cs, uint8_t rst, uint8_t busy, uint8_t dio1,
                           const LoRaRadioProfile& profile) = 0;

    virtual void deinitRfModule() = 0;

    virtual bool send(const uint8_t* data, size_t length) = 0;
    virtual bool startContinuousWave() = 0;
    virtual void stopContinuousWave() = 0;

    virtual bool startReceive(bool boosted = true) = 0;
    virtual int16_t pollReceive(std::vector<uint8_t>& payload) = 0;
    virtual void stopReceive() = 0;
    virtual bool isReceiving() const = 0;
    virtual int16_t receive(std::vector<uint8_t>& payload, uint32_t timeoutMs,
                            bool boosted = true, bool countTimeout = true) = 0;

    virtual bool setFrequency(float frequency) = 0;
    virtual bool setModemProfile(const LoRaRadioProfile& profile) = 0;
    virtual bool transmitFrame(const LoRaFrame& frame, bool& profileRestored) = 0;
    virtual LoRaRadioProfile getProfile() const = 0;
    virtual bool measureRssi(float frequency, uint32_t durationMs, RssiStats& stats) = 0;
    virtual bool runCad(bool& detected, uint32_t timeoutMs = 250) = 0;
    virtual uint32_t getTimeOnAir(size_t payloadLength) const = 0;

    virtual bool isInitialized() const = 0;
    virtual float getCurrentFrequency() const = 0;
    virtual float getRssi() const = 0;
    virtual float getSnr() const = 0;
    virtual size_t getLastPacketLength() const = 0;
    virtual int16_t getLastError() const = 0;

    virtual uint32_t getTxPackets() const = 0;
    virtual uint32_t getTxErrors() const = 0;
    virtual uint32_t getRxPackets() const = 0;
    virtual uint32_t getRxTimeouts() const = 0;
    virtual uint32_t getRxErrors() const = 0;
    virtual uint32_t getRxDropped() const = 0;
    virtual void resetStats() = 0;
};
