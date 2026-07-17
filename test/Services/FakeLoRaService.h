#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <vector>

#include "Interfaces/ILoRaService.h"

class FakeLoRaService final : public ILoRaService {
public:
    struct Configuration {
        uint8_t sck = 0;
        uint8_t miso = 0;
        uint8_t mosi = 0;
        uint8_t cs = 0;
        uint8_t rst = 0;
        uint8_t busy = 0;
        uint8_t dio1 = 0;
        LoRaRadioProfile profile;
    };

    struct Transmission {
        std::vector<uint8_t> payload;
    };

    struct PollResult {
        int16_t result = RECEIVE_TIMEOUT;
        std::vector<uint8_t> payload;
    };

    std::vector<Configuration> configurations;
    std::vector<Transmission> transmissions;
    std::vector<float> setFrequencies;
    std::vector<LoRaRadioProfile> modemProfiles;
    std::vector<LoRaFrame> transmittedFrames;
    std::vector<uint32_t> airtimeRequests;
    std::vector<std::pair<float, uint32_t>> rssiRequests;
    std::deque<PollResult> pollResults;
    std::deque<RssiStats> rssiResults;
    std::deque<bool> cadResults;

    bool configureResult = true;
    bool sendResult = true;
    bool startReceiveResult = true;
    bool startContinuousWaveResult = true;
    bool setFrequencyResult = true;
    bool setModemProfileResult = true;
    bool transmitFrameResult = true;
    bool profileRestoredOnTransmit = true;
    bool measureRssiResult = true;
    bool runCadResult = true;
    bool initialized = false;
    bool receiving = false;
    bool continuousWave = false;
    float rssi = -70.0f;
    float snr = 7.5f;
    size_t lastPacketLength = 0;
    int16_t lastError = 0;
    uint32_t txPackets = 0;
    uint32_t txErrors = 0;
    uint32_t rxPackets = 0;
    uint32_t rxTimeouts = 0;
    uint32_t rxErrors = 0;
    uint32_t rxDropped = 0;
    uint32_t stopReceiveCalls = 0;
    uint32_t deinitCalls = 0;
    uint32_t stopContinuousWaveCalls = 0;
    uint32_t resetStatsCalls = 0;
    uint32_t defaultAirtimeMs = 123;
    LoRaRadioProfile profile;

    bool configure(SPIClass&, uint8_t sck, uint8_t miso, uint8_t mosi,
                   uint8_t cs, uint8_t rst, uint8_t busy, uint8_t dio1,
                   const LoRaRadioProfile& nextProfile) override {
        configurations.push_back({sck, miso, mosi, cs, rst, busy, dio1, nextProfile});
        profile = nextProfile;
        initialized = configureResult;
        return configureResult;
    }

    void deinitRfModule() override {
        ++deinitCalls;
        initialized = false;
        receiving = false;
    }

    bool send(const uint8_t* data, size_t length) override {
        transmissions.push_back({std::vector<uint8_t>(data, data + length)});
        if (sendResult) ++txPackets;
        else ++txErrors;
        return sendResult;
    }

    bool startContinuousWave() override {
        continuousWave = startContinuousWaveResult;
        return startContinuousWaveResult;
    }

    void stopContinuousWave() override {
        ++stopContinuousWaveCalls;
        continuousWave = false;
    }

    bool startReceive(bool = true) override {
        receiving = startReceiveResult;
        return startReceiveResult;
    }

    int16_t pollReceive(std::vector<uint8_t>& payload) override {
        if (pollResults.empty()) {
            payload.clear();
            return RECEIVE_TIMEOUT;
        }

        const PollResult next = pollResults.front();
        pollResults.pop_front();
        payload = next.payload;
        if (next.result == RECEIVE_OK) {
            ++rxPackets;
            lastPacketLength = payload.size();
        } else if (next.result == RECEIVE_ERROR) {
            ++rxErrors;
        } else if (next.result == RECEIVE_TIMEOUT) {
            ++rxTimeouts;
        }
        return next.result;
    }

    void stopReceive() override {
        ++stopReceiveCalls;
        receiving = false;
    }

    bool isReceiving() const override { return receiving; }

    int16_t receive(std::vector<uint8_t>& payload, uint32_t, bool = true, bool = true) override {
        return pollReceive(payload);
    }

    bool setFrequency(float frequency) override {
        setFrequencies.push_back(frequency);
        if (setFrequencyResult) profile.frequency = frequency;
        return setFrequencyResult;
    }

    bool setModemProfile(const LoRaRadioProfile& nextProfile) override {
        modemProfiles.push_back(nextProfile);
        if (setModemProfileResult) profile = nextProfile;
        return setModemProfileResult;
    }

    bool transmitFrame(const LoRaFrame& frame, bool& profileRestored) override {
        transmittedFrames.push_back(frame);
        profileRestored = profileRestoredOnTransmit;
        return transmitFrameResult;
    }

    LoRaRadioProfile getProfile() const override { return profile; }

    bool measureRssi(float frequency, uint32_t durationMs, RssiStats& stats) override {
        rssiRequests.push_back({frequency, durationMs});
        if (!measureRssiResult) return false;
        if (rssiResults.empty()) {
            stats = {-90, -70, -80.0f, 1};
            return true;
        }
        stats = rssiResults.front();
        rssiResults.pop_front();
        return true;
    }

    bool runCad(bool& detected, uint32_t) override {
        if (!runCadResult) return false;
        detected = false;
        if (!cadResults.empty()) {
            detected = cadResults.front();
            cadResults.pop_front();
        }
        return true;
    }

    uint32_t getTimeOnAir(size_t payloadLength) const override {
        const_cast<FakeLoRaService*>(this)->airtimeRequests.push_back(static_cast<uint32_t>(payloadLength));
        return defaultAirtimeMs;
    }

    bool isInitialized() const override { return initialized; }
    float getCurrentFrequency() const override { return profile.frequency; }
    float getRssi() const override { return rssi; }
    float getSnr() const override { return snr; }
    size_t getLastPacketLength() const override { return lastPacketLength; }
    int16_t getLastError() const override { return lastError; }
    uint32_t getTxPackets() const override { return txPackets; }
    uint32_t getTxErrors() const override { return txErrors; }
    uint32_t getRxPackets() const override { return rxPackets; }
    uint32_t getRxTimeouts() const override { return rxTimeouts; }
    uint32_t getRxErrors() const override { return rxErrors; }
    uint32_t getRxDropped() const override { return rxDropped; }
    void resetStats() override { ++resetStatsCalls; }
};
