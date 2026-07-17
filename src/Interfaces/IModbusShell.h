#pragma once

#include <cstdint>
#include <string>

class IModbusShell {
public:
    virtual ~IModbusShell() = default;

    virtual void run(const std::string& host, uint16_t port) = 0;
};

