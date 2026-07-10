#pragma once

#ifdef DEVICE_CUSTOM

#include "Interfaces/IInput.h"
#include "Boards/Custom/CustomBoardConfig.h"
#include <Arduino.h>

class CustomInput : public IInput {
public:
    CustomInput();

    char mapButton();
    char readChar();
    char handler();
    void waitPress(uint32_t timeoutMs);
};

#endif
