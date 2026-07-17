#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <string>
#include <vector>

#include "Interfaces/IOneWireService.h"

class FakeOneWireService final : public IOneWireService {
public:
    std::vector<uint8_t> configuredPins;
    std::vector<uint8_t> configuredEepromPins;
    uint32_t closeCalls = 0;
    uint32_t closeEepromCalls = 0;
    uint32_t beginPassiveSniffCalls = 0;
    uint32_t resetSearchCalls = 0;
    uint32_t skipCalls = 0;
    std::vector<uint8_t> writes;
    std::vector<std::vector<uint8_t>> writeBytesCalls;
    std::vector<std::array<uint8_t, 8>> selectedRoms;
    std::vector<std::array<uint8_t, 8>> searchRoms;
    size_t searchIndex = 0;
    std::deque<bool> resetResults;
    bool defaultResetResult = false;
    std::deque<uint8_t> readResults;
    std::deque<std::vector<uint8_t>> readBytesResults;
    std::string byteCodeResult;
    std::vector<ByteCode> lastBytecodes;
    uint32_t writeRw1990Calls = 0;
    uint8_t lastRw1990Pin = 0;
    std::vector<uint8_t> lastRw1990Data;

    void configure(uint8_t pin) override { configuredPins.push_back(pin); }
    void close() override { ++closeCalls; }
    void beginPassiveSniff() override { ++beginPassiveSniffCalls; }
    int readPinLevel() const override { return 1; }

    bool reset() override {
        if (resetResults.empty()) return defaultResetResult;
        const bool result = resetResults.front();
        resetResults.pop_front();
        return result;
    }

    void write(uint8_t data) override { writes.push_back(data); }

    void writeBytes(const uint8_t* data, uint8_t len) override {
        writeBytesCalls.emplace_back(data, data + len);
    }

    uint8_t read() override {
        if (readResults.empty()) return 0;
        const uint8_t value = readResults.front();
        readResults.pop_front();
        return value;
    }

    void readBytes(uint8_t* buffer, uint8_t length) override {
        std::vector<uint8_t> data;
        if (!readBytesResults.empty()) {
            data = readBytesResults.front();
            readBytesResults.pop_front();
        }
        for (uint8_t i = 0; i < length; ++i) {
            buffer[i] = i < data.size() ? data[i] : 0;
        }
    }

    void skip() override { ++skipCalls; }

    void select(const uint8_t rom[8]) override {
        std::array<uint8_t, 8> copy{};
        for (size_t i = 0; i < copy.size(); ++i) copy[i] = rom[i];
        selectedRoms.push_back(copy);
    }

    uint8_t crc8(const uint8_t* data, uint8_t len) override { return data[len]; }

    void resetSearch() override {
        ++resetSearchCalls;
        searchIndex = 0;
    }

    bool search(uint8_t* rom) override {
        if (searchIndex >= searchRoms.size()) return false;
        const auto& next = searchRoms[searchIndex++];
        for (size_t i = 0; i < next.size(); ++i) rom[i] = next[i];
        return true;
    }

    std::string executeByteCode(const std::vector<ByteCode>& bytecodes) override {
        lastBytecodes = bytecodes;
        return byteCodeResult;
    }

    void writeRw1990(uint8_t pin, uint8_t* data, size_t len) override {
        ++writeRw1990Calls;
        lastRw1990Pin = pin;
        lastRw1990Data.assign(data, data + len);
    }

    void configureEeprom(uint8_t pin) override { configuredEepromPins.push_back(pin); }
    void closeEeprom() override { ++closeEepromCalls; }
    bool getEepromModelInfos(uint8_t*, std::string&, uint16_t&, uint8_t&) override { return false; }

    bool eeprom2431Probe(uint8_t* = nullptr) override { return false; }
    uint8_t eeprom2431ReadByte(uint16_t) override { return 0; }
    std::vector<uint8_t> eeprom2431Dump(uint16_t, uint16_t) override { return {}; }
    bool eeprom2431WriteRow(uint8_t, const uint8_t*, bool = true) override { return false; }
};
