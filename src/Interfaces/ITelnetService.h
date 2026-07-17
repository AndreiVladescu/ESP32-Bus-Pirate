#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

class ITelnetService {
public:
    virtual ~ITelnetService() = default;

    virtual bool connectTo(const std::string& host, uint16_t port, uint32_t recvTimeoutMs = 3000) = 0;
    virtual void close() = 0;
    virtual bool isConnected() const = 0;
    virtual bool writeChar(char c) = 0;
    virtual int writeRaw(const char* data, size_t len) = 0;
    virtual bool writeLine(const std::string& line) = 0;
    virtual void poll() = 0;
    virtual std::string readOutputNonBlocking() = 0;
    virtual const std::string& lastError() const = 0;
};
