#pragma once

#include <cstdint>
#include <deque>
#include <string>
#include <vector>

#include "Interfaces/IHdUartService.h"

class FakeHdUartService final : public IHdUartService {
public:
    struct Configuration {
        unsigned long baud = 0;
        uint8_t dataBits = 0;
        char parity = '\0';
        uint8_t stopBits = 0;
        uint8_t ioPin = 0;
        bool inverted = false;
    };

    mutable std::deque<char> rxData;
    std::vector<uint8_t> byteWrites;
    std::vector<std::string> stringWrites;
    std::vector<Configuration> configurations;
    std::vector<ByteCode> lastBytecodes;
    std::string byteCodeResult;
    uint32_t flushCalls = 0;
    uint32_t endCalls = 0;
    bool echoWrites = false;

    void configure(unsigned long baud, uint8_t dataBits, char parity,
                   uint8_t stopBits, uint8_t ioPin, bool inverted) override {
        configurations.push_back({baud, dataBits, parity, stopBits, ioPin, inverted});
    }

    void write(uint8_t data) override {
        byteWrites.push_back(data);
        if (echoWrites) rxData.push_back(static_cast<char>(data));
    }

    void write(const std::string& data) override { stringWrites.push_back(data); }
    bool available() const override { return !rxData.empty(); }

    char read() override {
        if (rxData.empty()) return '\0';
        const char value = rxData.front();
        rxData.pop_front();
        return value;
    }

    std::string readLine() override {
        std::string result;
        while (!rxData.empty()) {
            const char value = read();
            if (value == '\r' || value == '\n') break;
            result += value;
        }
        return result;
    }

    std::string executeByteCode(const std::vector<ByteCode>& bytecodes) override {
        lastBytecodes = bytecodes;
        return byteCodeResult;
    }

    void flush() override { ++flushCalls; }
    void end() override { ++endCalls; }

    void queueRx(const std::string& data) {
        for (const char value : data) rxData.push_back(value);
    }
};
