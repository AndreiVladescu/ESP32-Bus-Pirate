#pragma once

#include <cstdint>
#include <deque>
#include <string>
#include <utility>
#include <vector>

#include "Interfaces/ITwoWireService.h"

class FakeTwoWireService final : public ITwoWireService {
public:
    struct Configuration {
        uint8_t clk = 0;
        uint8_t io = 0;
        uint8_t rst = 0;
    };

    std::vector<Configuration> configurations;
    uint32_t endCalls = 0;
    uint32_t releaseSnifferCalls = 0;
    uint32_t startSnifferCalls = 0;
    uint32_t stopSnifferCalls = 0;
    bool startSnifferResult = true;
    std::deque<std::pair<uint8_t, uint8_t>> sniffEvents;

    void configure(uint8_t clkPin, uint8_t ioPin, uint8_t rstPin) override {
        configurations.push_back({clkPin, ioPin, rstPin});
    }

    void end() override { ++endCalls; }

    void setRST(bool) override {}
    void setCLK(bool) override {}
    void setIO(bool) override {}
    bool readIO() override { return false; }

    void pulseClock() override {}
    void sendClocks(uint16_t) override {}
    bool waitIOHigh(uint32_t) override { return false; }

    void writeBit(bool) override {}
    bool readBit() override { return false; }
    void writeByte(uint8_t) override {}
    uint8_t readByte() override { return 0; }

    void sendStart() override {}
    void sendStop() override {}
    void sendCommand(uint8_t, uint8_t, uint8_t) override {}
    std::vector<uint8_t> readResponse(uint16_t) override { return {}; }

    std::vector<uint8_t> performSmartCardAtr() override { return {}; }
    std::string parseSmartCardAtr(const std::vector<uint8_t>&) override { return ""; }
    uint8_t parseSmartCardRemainingAttempts(uint8_t) override { return 0; }
    std::string parseSmartCardStructureIdentifier(uint8_t) override { return ""; }
    std::vector<uint8_t> dumpSmartCardFullMemory() override { return {}; }
    void resetSmartCard() override {}
    void updateSmartCardSecurityAttempts(uint8_t) override {}
    void compareSmartCardVerificationData(uint8_t, uint8_t) override {}
    void writeSmartCardSecurityMemory(uint8_t, uint8_t) override {}
    void writeSmartCardProtectionMemory(uint8_t, uint8_t) override {}
    bool writeSmartCardMainMemory(uint8_t, uint8_t) override { return false; }
    std::vector<uint8_t> readSmartCardSecurityMemory() override { return {}; }
    std::vector<uint8_t> readSmartCardMainMemory(uint8_t, uint16_t) override { return {}; }
    std::vector<uint8_t> readSmartCardProtectionMemory() override { return {}; }
    bool protectSmartCard() override { return false; }
    bool unlockSmartCard(const uint8_t[3]) override { return false; }
    bool updateSmartCardPSC(const uint8_t[3]) override { return false; }
    bool getSmartCardPSC(uint8_t[3]) override { return false; }

    bool startSniffer() override {
        ++startSnifferCalls;
        return startSnifferResult;
    }

    void stopSniffer() override { ++stopSnifferCalls; }
    void releaseSniffer() override { ++releaseSnifferCalls; }

    bool getNextSniffEvent(uint8_t& type, uint8_t& data) override {
        if (sniffEvents.empty()) return false;
        type = sniffEvents.front().first;
        data = sniffEvents.front().second;
        sniffEvents.pop_front();
        return true;
    }

    void printSniffOnce(Stream&) override {}
};
