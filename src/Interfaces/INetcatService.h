#pragma once

#include <cstdint>
#include <string>

class INetcatService {
public:
    virtual ~INetcatService() = default;

    virtual void startTask(const std::string& host, int verbosity, uint16_t port, bool lineBuffer = false) = 0;
    virtual bool isConnected() const = 0;
    virtual void writeChar(char c) = 0;
    virtual std::string readOutputNonBlocking() = 0;
    virtual void close() = 0;
};

