#pragma once

#include "Interfaces/IInput.h"
#include "Arduino.h"

class DefaultInput final : public IInput {
public:
    DefaultInput();

    char mapButton();
    char readChar();
    char handler();
    void waitPress(uint32_t timeoutMs);
};
