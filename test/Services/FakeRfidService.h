#pragma once

#include <string>
#include <vector>

#include "Interfaces/IRfidService.h"

class FakeRfidService final : public IRfidService {
public:
    struct Configuration {
        uint8_t sda = 0;
        uint8_t scl = 0;
    };

    Configuration lastConfiguration;
    std::string uidValue = "04 A2 1B 00";
    std::string sakValue = "08";
    std::string atqaValue = "00 04";
    std::string piccTypeValue = "MIFARE 1K";
    std::string loadedDump;
    std::string uidSet;
    std::string sakSet;
    std::string atqaSet;
    std::string failureMessage = "RFID operation failed";
    std::vector<std::string> tagTypes = {" MIFARE / ISO14443A", " FeliCa"};
    std::vector<std::string> mifareFamilies = {" MIFARE Classic (16 bytes)", " NTAG/Ultralight (4 bytes)"};
    bool beginResult = true;
    RfidResult readResult = RfidResult::TagNotPresent;
    RfidResult writeResult = RfidResult::TagNotPresent;
    RfidResult eraseResult = RfidResult::TagNotPresent;
    RfidResult cloneResult = RfidResult::TagNotPresent;
    uint32_t configureCalls = 0;
    uint32_t beginCalls = 0;
    uint32_t readCalls = 0;
    uint32_t writeCalls = 0;
    uint32_t eraseCalls = 0;
    uint32_t cloneCalls = 0;
    uint32_t parseDataCalls = 0;
    bool lastCloneChecksSak = true;
    int lastReadMode = 0;
    int lastWriteMode = 0;

    void configure(uint8_t sda, uint8_t scl) override {
        lastConfiguration = {sda, scl};
        ++configureCalls;
    }

    bool begin() override {
        ++beginCalls;
        return beginResult;
    }

    RfidResult read(int cardBaudRate) override {
        lastReadMode = cardBaudRate;
        ++readCalls;
        return readResult;
    }

    RfidResult write(int cardBaudRate) override {
        lastWriteMode = cardBaudRate;
        ++writeCalls;
        return writeResult;
    }

    RfidResult erase() override {
        ++eraseCalls;
        return eraseResult;
    }

    RfidResult clone(bool checkSak = true) override {
        lastCloneChecksSak = checkSak;
        ++cloneCalls;
        return cloneResult;
    }

    std::string uid() const override { return uidValue; }
    std::string sak() const override { return sakValue; }
    std::string atqa() const override { return atqaValue; }
    std::string piccType() const override { return piccTypeValue; }
    void setUid(const std::string& value) override { uidSet = value; }
    void setSak(const std::string& value) override { sakSet = value; }
    void setAtqa(const std::string& value) override { atqaSet = value; }
    void loadDump(const std::string& dump) override { loadedDump = dump; }
    void parseData() override { ++parseDataCalls; }

    std::vector<std::string> getTagTypes() const override { return tagTypes; }
    std::vector<std::string> getMifareFamily() const override { return mifareFamilies; }
    std::string statusMessage(RfidResult result) const override {
        return result == RfidResult::Success ? "Success" : failureMessage;
    }
};
