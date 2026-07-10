#ifdef DEVICE_CUSTOM

#include "Boards/Custom/CustomInput.h"
#include "Data/InputKeys.h"

CustomInput::CustomInput() {
#if CUSTOM_INPUT_BUTTON_PIN >= 0
  #if CUSTOM_INPUT_BUTTON_PULLUP
    pinMode(CUSTOM_INPUT_BUTTON_PIN, INPUT_PULLUP);
  #else
    pinMode(CUSTOM_INPUT_BUTTON_PIN, INPUT);
  #endif
#endif
}

char CustomInput::mapButton() {
#if CUSTOM_INPUT_BUTTON_PIN >= 0
    const int state = digitalRead(CUSTOM_INPUT_BUTTON_PIN);
    const bool pressed = CUSTOM_INPUT_BUTTON_ACTIVE_LOW ? (state == LOW) : (state == HIGH);
    return pressed ? KEY_OK : KEY_NONE;
#else
    return KEY_NONE;
#endif
}

char CustomInput::readChar() {
    return mapButton();
}

char CustomInput::handler() {
#if CUSTOM_INPUT_BUTTON_PIN < 0
    return KEY_OK;
#else
    char c = KEY_NONE;
    while ((c = mapButton()) == KEY_NONE) {
        delay(5);
    }
    return c;
#endif
}

void CustomInput::waitPress(uint32_t timeoutMs) {
#if CUSTOM_INPUT_BUTTON_PIN < 0
    (void)timeoutMs;
#else
    const uint32_t start = millis();
    while (mapButton() == KEY_NONE) {
        if (timeoutMs > 0 && millis() - start >= timeoutMs) {
            return;
        }
        delay(5);
    }
#endif
}

#endif
