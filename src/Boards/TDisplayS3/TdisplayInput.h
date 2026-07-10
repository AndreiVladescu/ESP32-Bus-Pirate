#pragma once

#if defined(DEVICE_TDISPLAYS3)

#include "Interfaces/IInput.h"
#include <Arduino.h>

#define TDISPLAY_BTN_UP           0
#define TDISPLAY_BTN_DOWN        14

class TdisplayInput : public IInput {
public:
    TdisplayInput();

    char handler() override;
    char readChar() override;
    void waitPress(uint32_t timeoutMs) override;

    void tick();
    void checkShutdownRequest();
    void shutdownToDeepSleep();

private:
    uint8_t readButtons();
    uint8_t checkLongPress();
    char lastInput;
    char lastButton;
    int lastPos;
    const uint8_t BTN_UP = 1;
    const uint8_t BTN_DOWN = 2;
    const uint8_t BTN_LONG = 4;
    const uint8_t BTN_SHUT = 8;
    const uint8_t LONG_PRESS_MIN = 5;
    const uint8_t LONG_PRESS_MAX = 20;
    const uint8_t LONG_PRESS_MIN_SHTDWN = 50;
};

#endif
