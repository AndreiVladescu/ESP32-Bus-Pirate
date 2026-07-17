#pragma once

#include <cstdint>

#include "Interfaces/IShell.h"

class FakeShell final : public IShell {
public:
    uint32_t runCalls = 0;

    void run() override { ++runCalls; }
};
