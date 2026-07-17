#pragma once

#include <cstdint>

class HardwareSerial;

class IUartPort {
public:
    virtual ~IUartPort() = default;

    virtual void configure(unsigned long baud, uint32_t config, uint8_t rx, uint8_t tx,
                           bool inverted, HardwareSerial* serial = nullptr,
                           bool noAllocation = false) = 0;
    virtual char read() = 0;
    virtual bool available() const = 0;
    virtual void setRxFIFOFull(uint8_t fifoBytes) = 0;
    virtual void flush() = 0;
    virtual void end() = 0;
};
