#pragma once

#include <cstdint>
#include <string>
#include <vector>

class Stream;

class ITwoWireService {
public:
    virtual ~ITwoWireService() = default;

    virtual void configure(uint8_t clkPin, uint8_t ioPin, uint8_t rstPin) = 0;
    virtual void end() = 0;

    virtual void setRST(bool level) = 0;
    virtual void setCLK(bool level) = 0;
    virtual void setIO(bool level) = 0;
    virtual bool readIO() = 0;

    virtual void pulseClock() = 0;
    virtual void sendClocks(uint16_t ticks) = 0;
    virtual bool waitIOHigh(uint32_t maxTicks) = 0;

    virtual void writeBit(bool bit) = 0;
    virtual bool readBit() = 0;
    virtual void writeByte(uint8_t byte) = 0;
    virtual uint8_t readByte() = 0;

    virtual void sendStart() = 0;
    virtual void sendStop() = 0;
    virtual void sendCommand(uint8_t a, uint8_t b, uint8_t c) = 0;
    virtual std::vector<uint8_t> readResponse(uint16_t len) = 0;

    virtual std::vector<uint8_t> performSmartCardAtr() = 0;
    virtual std::string parseSmartCardAtr(const std::vector<uint8_t>& atr) = 0;
    virtual uint8_t parseSmartCardRemainingAttempts(uint8_t statusByte) = 0;
    virtual std::string parseSmartCardStructureIdentifier(uint8_t id) = 0;
    virtual std::vector<uint8_t> dumpSmartCardFullMemory() = 0;
    virtual void resetSmartCard() = 0;
    virtual void updateSmartCardSecurityAttempts(uint8_t pattern) = 0;
    virtual void compareSmartCardVerificationData(uint8_t address, uint8_t value) = 0;
    virtual void writeSmartCardSecurityMemory(uint8_t address, uint8_t value) = 0;
    virtual void writeSmartCardProtectionMemory(uint8_t address, uint8_t value) = 0;
    virtual bool writeSmartCardMainMemory(uint8_t address, uint8_t value) = 0;
    virtual std::vector<uint8_t> readSmartCardSecurityMemory() = 0;
    virtual std::vector<uint8_t> readSmartCardMainMemory(uint8_t startAddress, uint16_t length) = 0;
    virtual std::vector<uint8_t> readSmartCardProtectionMemory() = 0;
    virtual bool protectSmartCard() = 0;
    virtual bool unlockSmartCard(const uint8_t psc[3]) = 0;
    virtual bool updateSmartCardPSC(const uint8_t psc[3]) = 0;
    virtual bool getSmartCardPSC(uint8_t outPsc[3]) = 0;

    virtual bool startSniffer() = 0;
    virtual void stopSniffer() = 0;
    virtual void releaseSniffer() = 0;
    virtual bool getNextSniffEvent(uint8_t& type, uint8_t& data) = 0;
    virtual void printSniffOnce(Stream& out) = 0;
};
