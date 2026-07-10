#pragma once

#ifdef DEVICE_VISION_MASTER_T190

#include "Interfaces/IInput.h"

class VisionMasterT190Input final : public IInput {
public:
    VisionMasterT190Input();

    char readChar() override;
    char handler() override;
    void waitPress(uint32_t timeoutMs) override;

private:
    static constexpr uint8_t BUTTON_1_PIN = 0;
    static constexpr uint8_t BUTTON_2_PIN = 21;

    char readButtonEvent();

    bool button1WasPressed = false;
    bool button2WasPressed = false;
};

#endif
