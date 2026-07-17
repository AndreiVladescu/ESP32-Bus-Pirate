#pragma once

#include <string>
#include <vector>

#include "Interfaces/ISshService.h"

class FakeSshService final : public ISshService {
public:
    struct StartCall {
        std::string host;
        std::string user;
        std::string pass;
        int verbosity = 0;
        int port = 0;
    };

    bool connected = true;
    std::string output;
    std::vector<StartCall> startCalls;
    std::vector<char> writtenChars;
    uint32_t closeCalls = 0;

    void startTask(const std::string& host, const std::string& user, const std::string& pass, int verbosity, int port) override {
        startCalls.push_back({host, user, pass, verbosity, port});
    }

    bool isConnected() const override { return connected; }
    void writeChar(char c) override { writtenChars.push_back(c); }
    std::string readOutput() override { return output; }

    std::string readOutputNonBlocking() override {
        std::string out = output;
        output.clear();
        return out;
    }

    void close() override { ++closeCalls; connected = false; }
};

