#ifdef DEVICE_VISION_MASTER_T190

#include "Boards/VisionMasterT190/VisionMasterT190Input.h"
#include "Data/InputKeys.h"
#include <Arduino.h>

VisionMasterT190Input::VisionMasterT190Input() {
    pinMode(BUTTON_1_PIN, INPUT_PULLUP);
    pinMode(BUTTON_2_PIN, INPUT_PULLUP);
}

char VisionMasterT190Input::readButtonEvent() {
    const bool button1Pressed = digitalRead(BUTTON_1_PIN) == LOW;
    const bool button2Pressed = digitalRead(BUTTON_2_PIN) == LOW;
    char event = KEY_NONE;

    if (button1Pressed && !button1WasPressed) {
        event = KEY_OK;
    } else if (button2Pressed && !button2WasPressed) {
        event = KEY_ARROW_RIGHT;
    }

    button1WasPressed = button1Pressed;
    button2WasPressed = button2Pressed;
    return event;
}

char VisionMasterT190Input::readChar() {
    return readButtonEvent();
}

char VisionMasterT190Input::handler() {
    char key = KEY_NONE;
    while ((key = readButtonEvent()) == KEY_NONE) {
        delay(5);
    }
    return key;
}

void VisionMasterT190Input::waitPress(uint32_t timeoutMs) {
    const uint32_t start = millis();
    while (timeoutMs == 0 || millis() - start < timeoutMs) {
        if (readButtonEvent() != KEY_NONE) return;
        delay(5);
    }
}

#endif
