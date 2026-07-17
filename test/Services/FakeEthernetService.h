#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "Interfaces/IEthernetService.h"

class FakeEthernetService final : public IEthernetService {
public:
    struct ConfigureCall {
        int8_t cs = 0;
        int8_t rst = 0;
        int8_t sck = 0;
        int8_t miso = 0;
        int8_t mosi = 0;
        int8_t irq = 0;
        uint32_t spiHz = 0;
        std::array<uint8_t, 6> mac{};
        SPIClass* spi = nullptr;
        uint8_t phyAddr = 0;
    };

    bool configureResult = true;
    bool beginDhcpResult = true;
    bool connected = false;
    bool link = true;
    std::string macString = "DE:AD:BE:EF:00:42";
    std::string localIp = "10.0.0.42";
    std::string subnet = "255.255.255.0";
    std::string gateway = "10.0.0.1";
    std::string dns = "1.1.1.1";

    std::vector<ConfigureCall> configureCalls;
    std::vector<unsigned long> dhcpTimeouts;
    uint32_t hardResetCalls = 0;

    bool configure(int8_t pinCS,
                   int8_t pinRST,
                   int8_t pinSCK,
                   int8_t pinMISO,
                   int8_t pinMOSI,
                   int8_t pinIRQ,
                   uint32_t spiHz,
                   const std::array<uint8_t,6>& chosenMac,
                   SPIClass* spi = nullptr,
                   uint8_t phyAddr = 1) override {
        configureCalls.push_back({pinCS, pinRST, pinSCK, pinMISO, pinMOSI, pinIRQ, spiHz, chosenMac, spi, phyAddr});
        return configureResult;
    }

    bool beginDHCP(unsigned long timeoutMs) override {
        dhcpTimeouts.push_back(timeoutMs);
        connected = beginDhcpResult;
        return beginDhcpResult;
    }

    bool isConnected() const override { return connected; }
    bool linkUp() const override { return link; }
    std::string getMac() const override { return macString; }
    std::string getLocalIP() const override { return localIp; }
    std::string getSubnetMask() const override { return subnet; }
    std::string getGatewayIp() const override { return gateway; }
    std::string getDns() const override { return dns; }
    void hardReset() override { ++hardResetCalls; }
};

