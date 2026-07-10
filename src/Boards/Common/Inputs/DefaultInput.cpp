#include "Boards/Common/Inputs/DefaultInput.h"
#include "Data/InputKeys.h"

#define BOOT_BUTTON_PIN 0

DefaultInput::DefaultInput() {
    pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
}

char DefaultInput::mapButton() {
    if (digitalRead(BOOT_BUTTON_PIN) == LOW) {
        return KEY_OK;
    }
    return KEY_NONE;
}

char DefaultInput::readChar() {
    return mapButton();
}

char DefaultInput::handler() {
    char c = KEY_NONE;
    while ((c = mapButton()) == KEY_NONE) {
        delay(5);
    }
    return c;
}

void DefaultInput::waitPress(uint32_t timeoutMs) {
    (void)timeoutMs; // currently not used
    while (mapButton() == KEY_NONE) {
        delay(5);
    }
}
