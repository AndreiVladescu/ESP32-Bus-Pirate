#pragma once

#include <cstdint>
#include <deque>
#include <string>
#include <vector>

#include "Interfaces/IUartService.h"

class FakeUartService final : public IUartService {
public:
    struct Configuration {
        unsigned long baud = 0;
        uint32_t config = 0;
        uint8_t rx = 0;
        uint8_t tx = 0;
        bool inverted = false;
        HardwareSerial* serial = nullptr;
        bool noAllocation = false;
    };

    mutable std::deque<char> rxData;
    std::vector<std::string> writes;
    std::vector<Configuration> configurations;
    std::vector<ByteCode> lastBytecodes;
    std::vector<std::vector<uint8_t>> scanPinsCalls;
    std::vector<uint8_t> detectBaudPins;
    std::vector<unsigned long> switchedBauds;
    std::deque<std::vector<UartPinActivity>> scanResults;
    std::deque<uint32_t> detectBaudResults;
    std::vector<uint32_t> baudList{9600, 19200, 115200};
    std::string byteCodeResult;
    std::string responseAfterHandshake;
    uint32_t uartConfigResult = 0x800001c;
    uint8_t lastDataBits = 0;
    char lastParity = '\0';
    uint8_t lastStopBits = 0;
    uint32_t releaseCalls = 0;
    uint32_t flushCalls = 0;
    uint32_t clearCalls = 0;
    uint32_t endCalls = 0;
    uint32_t setRxFIFOFullCalls = 0;
    uint8_t lastRxFIFOFull = 0;
    std::string rxAfterSetRxFIFOFull;
    bool installed = false;

    void configure(unsigned long baud, uint32_t config, uint8_t rx, uint8_t tx,
                   bool inverted, HardwareSerial* serial = nullptr,
                   bool noAllocation = false) override {
        configurations.push_back({baud, config, rx, tx, inverted, serial, noAllocation});
        installed = true;
    }

    void release() override {
        ++releaseCalls;
        installed = false;
    }

    void print(const std::string& msg) override { writes.push_back(msg); }
    void println(const std::string& msg) override { writes.push_back(msg + "\n"); }

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

    bool available() const override { return !rxData.empty(); }

    void write(char value) override { writes.emplace_back(1, value); }

    void write(const char* value) override {
        write(std::string(value == nullptr ? "" : value));
    }

    void write(const std::string& value) override {
        writes.push_back(value);
        if (value == "handshake\n") queueRx(responseAfterHandshake);
    }

    void setRxFIFOFull(uint8_t fifoBytes) override {
        ++setRxFIFOFullCalls;
        lastRxFIFOFull = fifoBytes;
        if (!rxAfterSetRxFIFOFull.empty()) {
            queueRx(rxAfterSetRxFIFOFull);
            rxAfterSetRxFIFOFull.clear();
        }
    }
    void setDefaultRxFIFOFull() override {}

    std::string executeByteCode(const std::vector<ByteCode>& bytecodes) override {
        lastBytecodes = bytecodes;
        return byteCodeResult;
    }

    void switchBaudrate(unsigned long newBaud) override {
        switchedBauds.push_back(newBaud);
    }
    void flush() override { ++flushCalls; }
    void clearUartBuffer() override { ++clearCalls; }

    void end() override {
        ++endCalls;
        installed = false;
    }

    bool isInstalled() const override { return installed; }

    uint32_t buildUartConfig(uint8_t dataBits, char parity, uint8_t stopBits) override {
        lastDataBits = dataBits;
        lastParity = parity;
        lastStopBits = stopBits;
        return uartConfigResult;
    }

    void initXmodem() override {}
    bool xmodemReceiveToFile(fs::File&) override { return false; }
    bool xmodemSendFile(fs::File&) override { return false; }
    void setXmodemReceiveHandler(bool (*)(void*, size_t, uint8_t*, size_t)) override {}
    void setXmodemSendHandler(void (*)(void*, size_t, uint8_t*, size_t)) override {}
    void setXmodemBlockSize(int32_t) override {}
    void setXmodemIdSize(int8_t) override {}
    void setXmodemCrc(bool) override {}
    int32_t getXmodemBlockSize() const override { return 128; }
    int8_t getXmodemIdSize() const override { return 1; }

    UartPinActivity measureUartActivity(uint8_t, uint32_t = 100, bool = true) override {
        return {};
    }

    std::vector<UartPinActivity> scanUartActivity(const std::vector<uint8_t>& pins,
                                                  uint32_t = 100,
                                                  uint32_t = 10,
                                                  bool = true) override {
        scanPinsCalls.push_back(pins);
        if (scanResults.empty()) return {};
        auto result = scanResults.front();
        scanResults.pop_front();
        return result;
    }

    uint32_t detectBaudByEdge(uint8_t pin, uint32_t = 5000, uint32_t = 300,
                              uint32_t = 30, bool = true) override {
        detectBaudPins.push_back(pin);
        if (detectBaudResults.empty()) return 0;
        const uint32_t result = detectBaudResults.front();
        detectBaudResults.pop_front();
        return result;
    }

    std::vector<uint32_t> getBaudList() const override { return baudList; }

    void queueRx(const std::string& data) {
        for (const char value : data) rxData.push_back(value);
    }

    bool wrote(const std::string& value) const {
        for (const auto& write : writes) {
            if (write == value) return true;
        }
        return false;
    }
};
