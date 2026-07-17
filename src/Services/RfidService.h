#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <Wire.h>
#include "Interfaces/IRfidService.h"
#include "Vendors/PN532.h" 

class RfidService : public IRfidService {
public:
    // Base, I2C only
    void configure(uint8_t sda, uint8_t scl) override;
    void release();
    bool begin() override;

    // Operations
    RfidResult read(int cardBaudRate = 0) override;  // 0 = MIFARE, 1 = FeliCa
    RfidResult write(int cardBaudRate = 0) override;
    int  write_ndef();
    RfidResult erase() override;
    RfidResult clone(bool checkSak = true) override;

    // Getters
    std::string uid() const override;          // ex: "04 A2 1B ..."
    std::string sak() const override;          // ex: "08"
    std::string atqa() const override;         // ex: "00 04"
    std::string piccType() const override;     // ex: "MIFARE 1K"

    // Setters
    void  setUid(const std::string& uidHex) override;
    void  setSak(const std::string& sakHex) override;
    void  setAtqa(const std::string& atqaHex) override;
    
    // Pages
    int  totalPages() const;
    int  dataPages() const;
    bool pageReadOk() const;
    std::string allPages() const;     // dump
    void loadDump(const std::string& dump) override;

    // Helpers
    std::vector<std::string> getTagTypes() const override;
    std::vector<std::string> getMifareFamily() const override;
    std::string statusMessage(RfidResult result) const override;
    void parseData() override;

private:
    uint8_t  _sda   = 1;
    uint8_t  _scl   = 2;
    bool     _configured = false;
    bool     _begun      = false;

    PN532* _rfid = nullptr;
};
