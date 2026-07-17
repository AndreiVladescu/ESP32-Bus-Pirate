#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "Interfaces/IUartPort.h"
#include "Models/ByteCode.h"

namespace fs { class File; }

struct UartPinActivity {
    uint8_t pin;
    uint32_t edges;
    float edgesPerSec;
    uint32_t approxBaud;
};

class IUartService : public IUartPort {
public:
    virtual ~IUartService() = default;

    virtual void configure(unsigned long baud, uint32_t config, uint8_t rx, uint8_t tx,
                           bool inverted, HardwareSerial* serial = nullptr,
                           bool noAllocation = false) override = 0;
    virtual void release() = 0;
    virtual void print(const std::string& msg) = 0;
    virtual void println(const std::string& msg) = 0;
    virtual char read() override = 0;
    virtual std::string readLine() = 0;
    virtual bool available() const override = 0;
    virtual void write(char c) = 0;
    virtual void write(const char* str) = 0;
    virtual void write(const std::string& str) = 0;
    virtual void setRxFIFOFull(uint8_t fifoBytes) override = 0;
    virtual void setDefaultRxFIFOFull() = 0;
    virtual std::string executeByteCode(const std::vector<ByteCode>& bytecodes) = 0;
    virtual void switchBaudrate(unsigned long newBaud) = 0;
    virtual void flush() override = 0;
    virtual void clearUartBuffer() = 0;
    virtual void end() override = 0;
    virtual bool isInstalled() const = 0;
    virtual uint32_t buildUartConfig(uint8_t dataBits, char parity, uint8_t stopBits) = 0;

    virtual void initXmodem() = 0;
    virtual bool xmodemReceiveToFile(fs::File& file) = 0;
    virtual bool xmodemSendFile(fs::File& file) = 0;
    virtual void setXmodemReceiveHandler(bool (*handler)(void*, size_t, uint8_t*, size_t)) = 0;
    virtual void setXmodemSendHandler(void (*handler)(void*, size_t, uint8_t*, size_t)) = 0;
    virtual void setXmodemBlockSize(int32_t size) = 0;
    virtual void setXmodemIdSize(int8_t size) = 0;
    virtual void setXmodemCrc(bool enabled) = 0;
    virtual int32_t getXmodemBlockSize() const = 0;
    virtual int8_t getXmodemIdSize() const = 0;

    virtual UartPinActivity measureUartActivity(uint8_t pin, uint32_t windowMs = 100,
                                                bool pullup = true) = 0;
    virtual std::vector<UartPinActivity> scanUartActivity(const std::vector<uint8_t>& pins,
                                                          uint32_t windowMs = 100,
                                                          uint32_t minEdges = 10,
                                                          bool pullup = true) = 0;
    virtual uint32_t detectBaudByEdge(uint8_t pin, uint32_t totalMs = 5000,
                                      uint32_t windowMs = 300, uint32_t minEdges = 30,
                                      bool pullup = true) = 0;
    virtual std::vector<uint32_t> getBaudList() const = 0;
};
