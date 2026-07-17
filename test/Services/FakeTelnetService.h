#pragma once

#include <string>
#include <vector>

#include "Interfaces/ITelnetService.h"

class FakeTelnetService final : public ITelnetService {
public:
    struct ConnectCall {
        std::string host;
        uint16_t port = 0;
        uint32_t timeoutMs = 0;
    };

    bool connectResult = true;
    bool connected = false;
    std::string output;
    std::string error = "boom";
    std::vector<ConnectCall> connectCalls;
    std::vector<char> writtenChars;
    std::vector<std::string> writtenLines;
    uint32_t closeCalls = 0;
    uint32_t pollCalls = 0;

    bool connectTo(const std::string& host, uint16_t port, uint32_t recvTimeoutMs = 3000) override {
        connectCalls.push_back({host, port, recvTimeoutMs});
        connected = connectResult;
        return connectResult;
    }

    void close() override {
        ++closeCalls;
        connected = false;
    }

    bool isConnected() const override { return connected; }

    bool writeChar(char c) override {
        writtenChars.push_back(c);
        return true;
    }

    int writeRaw(const char*, size_t len) override { return static_cast<int>(len); }

    bool writeLine(const std::string& line) override {
        writtenLines.push_back(line);
        return true;
    }

    void poll() override { ++pollCalls; }

    std::string readOutputNonBlocking() override {
        std::string out = output;
        output.clear();
        return out;
    }

    const std::string& lastError() const override { return error; }
};

