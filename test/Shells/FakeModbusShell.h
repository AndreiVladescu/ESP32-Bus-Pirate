#pragma once

#include <string>
#include <vector>

#include "Interfaces/IModbusShell.h"

class FakeModbusShell final : public IModbusShell {
public:
    struct RunCall {
        std::string host;
        uint16_t port = 0;
    };

    std::vector<RunCall> runCalls;

    void run(const std::string& host, uint16_t port) override {
        runCalls.push_back({host, port});
    }
};

