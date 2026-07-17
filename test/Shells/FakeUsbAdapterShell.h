#pragma once

#include <cstdint>

#include "Interfaces/IUsbAdapterShell.h"

class FakeUsbAdapterShell final : public IUsbAdapterShell {
public:
    uint32_t runCalls = 0;
    uint32_t rebootOpenOcdCalls = 0;

    void run() override {
        ++runCalls;
    }

    void rebootOpenOcdBusPirate() override {
        ++rebootOpenOcdCalls;
    }
};
