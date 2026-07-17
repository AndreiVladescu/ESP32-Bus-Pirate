#pragma once

#include <string>
#include <vector>

#include "Interfaces/INetcatService.h"

class FakeNetcatService final : public INetcatService {
public:
    struct StartCall {
        std::string host;
        int verbosity = 0;
        uint16_t port = 0;
        bool lineBuffer = false;
    };

    bool connected = true;
    std::string output;
    std::vector<StartCall> startCalls;
    std::vector<char> writtenChars;
    uint32_t closeCalls = 0;

    void startTask(const std::string& host, int verbosity, uint16_t port, bool lineBuffer = false) override {
        startCalls.push_back({host, verbosity, port, lineBuffer});
    }

    bool isConnected() const override { return connected; }
    void writeChar(char c) override { writtenChars.push_back(c); }

    std::string readOutputNonBlocking() override {
        std::string out = output;
        output.clear();
        return out;
    }

    void close() override { ++closeCalls; connected = false; }
};

