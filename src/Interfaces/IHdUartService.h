#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Models/ByteCode.h"

class IHdUartService {
public:
    virtual ~IHdUartService() = default;

    virtual void configure(unsigned long baud, uint8_t dataBits, char parity,
                           uint8_t stopBits, uint8_t ioPin, bool inverted) = 0;
    virtual void write(uint8_t data) = 0;
    virtual void write(const std::string& str) = 0;
    virtual bool available() const = 0;
    virtual char read() = 0;
    virtual std::string readLine() = 0;
    virtual std::string executeByteCode(const std::vector<ByteCode>& bytecodes) = 0;
    virtual void flush() = 0;
    virtual void end() = 0;
};
