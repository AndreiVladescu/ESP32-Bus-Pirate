#ifdef DEVICE_M5STAMPS3

#include "Boards/StampS3/StampS3Input.h"
#include "Data/InputKeys.h"

StampS3Input::StampS3Input() = default;

char StampS3Input::mapButton() {
    M5.update();

    if (M5.BtnA.wasPressed()) return KEY_OK;

    return KEY_NONE;
}

char StampS3Input::readChar() {
    M5.update();
    if (M5.BtnA.isPressed()) return KEY_OK;
    return KEY_NONE;
}

char StampS3Input::handler() {
    char c = KEY_NONE;
    while ((c = mapButton()) == KEY_NONE) {
        delay(10);
    }
    return c;
}

void StampS3Input::waitPress(uint32_t timeoutMs) {
    (void)timeoutMs; // currently not used
    while (mapButton() == KEY_NONE) {
        delay(10);
    }
}

#endif
