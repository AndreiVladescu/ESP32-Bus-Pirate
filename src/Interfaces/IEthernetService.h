#pragma once

#include <array>
#include <cstdint>
#include <string>

class SPIClass;

class IEthernetService {
public:
    virtual ~IEthernetService() = default;

    virtual bool configure(int8_t pinCS,
                           int8_t pinRST,
                           int8_t pinSCK,
                           int8_t pinMISO,
                           int8_t pinMOSI,
                           int8_t pinIRQ,
                           uint32_t spiHz,
                           const std::array<uint8_t,6>& chosenMac,
                           SPIClass* spi = nullptr,
                           uint8_t phyAddr = 1) = 0;

    virtual bool beginDHCP(unsigned long timeoutMs) = 0;
    virtual bool isConnected() const = 0;
    virtual bool linkUp() const = 0;
    virtual std::string getMac() const = 0;
    virtual std::string getLocalIP() const = 0;
    virtual std::string getSubnetMask() const = 0;
    virtual std::string getGatewayIp() const = 0;
    virtual std::string getDns() const = 0;
    virtual void hardReset() = 0;
};

