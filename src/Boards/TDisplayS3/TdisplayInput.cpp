#if defined(DEVICE_TDISPLAYS3)

#include "TdisplayInput.h"
#include "Data/InputKeys.h"
#include <esp_sleep.h>
#include <Arduino.h>

TdisplayInput::TdisplayInput()
    : lastInput(KEY_NONE),
      lastButton(0)
{
    pinMode(TDISPLAY_BTN_UP, INPUT_PULLUP);
    pinMode(TDISPLAY_BTN_DOWN, INPUT_PULLUP);
}

uint8_t TdisplayInput::checkLongPress(){
    uint8_t iter = 0;
    while ((digitalRead(TDISPLAY_BTN_DOWN) == LOW  || digitalRead(TDISPLAY_BTN_UP) == LOW) && iter < LONG_PRESS_MIN_SHTDWN ){
        delay(100);
        iter++;
    }
    return iter;
}

uint8_t TdisplayInput::readButtons(){
    uint8_t down = digitalRead(TDISPLAY_BTN_DOWN) == LOW ? BTN_DOWN : 0;
    uint8_t up   = digitalRead(TDISPLAY_BTN_UP) == LOW ? BTN_UP : 0;
    uint8_t longPress = 0;
    
    uint8_t longTime = checkLongPress();
    
    if ((longTime > LONG_PRESS_MIN) && longTime < LONG_PRESS_MAX){
        longPress = BTN_LONG;
    } else if (longTime >= LONG_PRESS_MIN_SHTDWN){
        longPress = BTN_SHUT;
    }
    return( down + up + longPress);
}

void TdisplayInput::tick() {

    int buttons = readButtons();
    if ( buttons != lastButton){
        if (buttons == BTN_UP) {
            lastInput = KEY_ARROW_LEFT;
        } else if (buttons == BTN_DOWN) {
            lastInput = KEY_ARROW_RIGHT;
        } else if (buttons == (BTN_UP + BTN_LONG)) {
            lastInput = KEY_OK;
        } else if (buttons == (BTN_DOWN + BTN_LONG)) {
            lastInput = KEY_ARROW_DOWN;
        } else if (buttons && BTN_SHUT){
            shutdownToDeepSleep();
        }
        lastButton = buttons;
    }
}

char TdisplayInput::readChar() {
    tick();
    char c = lastInput;
    lastInput = KEY_NONE;
    return c;
}

char TdisplayInput::handler() {
    while (true) {
        char c = readChar();
        if (c != KEY_NONE) return c;
        delay(5);
    }
}

void TdisplayInput::waitPress(uint32_t timeoutMs) {
    uint32_t start = millis();
    while (true) {
        if (readChar() != KEY_NONE) return;
        if (timeoutMs > 0 && (millis() - start) >= timeoutMs) return;
        delay(5);
    }
}

void TdisplayInput::shutdownToDeepSleep() {
    //view.shutDown();
    delay(3000);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)TDISPLAY_BTN_DOWN, 0);
    esp_deep_sleep_start();
}

#endif
