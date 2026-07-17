#pragma once

#include <cstdint>
#include <string>
#include <vector>

enum class RfidResult : int {
    Success = 0,
    Failure = 1,
    TagNotPresent = 2,
    TagNotMatch = 3,
    AuthenticationError = 4,
    NotImplemented = 5,
};

// Capabilities required by RfidController.
// The interface intentionally exposes semantic results, never PN532 types.
class IRfidService {
public:
    virtual ~IRfidService() = default;

    virtual void configure(uint8_t sda, uint8_t scl) = 0;
    virtual bool begin() = 0;

    virtual RfidResult read(int cardBaudRate) = 0;
    virtual RfidResult write(int cardBaudRate) = 0;
    virtual RfidResult erase() = 0;
    virtual RfidResult clone(bool checkSak = true) = 0;

    virtual std::string uid() const = 0;
    virtual std::string sak() const = 0;
    virtual std::string atqa() const = 0;
    virtual std::string piccType() const = 0;
    virtual void setUid(const std::string& uidHex) = 0;
    virtual void setSak(const std::string& sakHex) = 0;
    virtual void setAtqa(const std::string& atqaHex) = 0;
    virtual void loadDump(const std::string& dump) = 0;
    virtual void parseData() = 0;

    virtual std::vector<std::string> getTagTypes() const = 0;
    virtual std::vector<std::string> getMifareFamily() const = 0;
    virtual std::string statusMessage(RfidResult result) const = 0;
};
