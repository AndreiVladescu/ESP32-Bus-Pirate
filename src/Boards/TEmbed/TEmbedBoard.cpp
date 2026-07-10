#if defined(DEVICE_TEMBEDS3) || defined(DEVICE_TEMBEDS3CC1101)

#include "Boards/TEmbed/TEmbedBoard.h"
#include <Arduino.h>

St7789SpiConfig TEmbedBoard::createDisplayConfig() {
    St7789SpiConfig config;

    #ifdef DEVICE_TEMBEDS3CC1101
        config.pinBacklight = 21;
        config.pinMiso = 10;
        config.pinMosi = 9;
        config.pinSclk = 11;
        config.pinCs = 41;
        config.pinDc = 16;
        config.pinReset = -1;
        config.useSharedSpi = false;
    #else
        config.pinBacklight = 15;
        config.pinMiso = -1;
        config.pinMosi = 11;
        config.pinSclk = 12;
        config.pinCs = 10;
        config.pinDc = 13;
        config.pinReset = 9;
    #endif

    config.pinPower = 46;
    config.panelWidth = 170;
    config.panelHeight = 320;
    config.memoryWidth = 240;
    config.memoryHeight = 320;
    config.offsetX = 35;
    config.offsetY = 0;
    config.selectionHelpLine1 = "Long press button to shut down";
    return config;
}

void TEmbedBoard::initializeBoardPeripherals() {
    #ifdef DEVICE_TEMBEDS3CC1101
        constexpr uint8_t peripheralPowerPin = 15;
        pinMode(peripheralPowerPin, OUTPUT);
        digitalWrite(peripheralPowerPin, HIGH);
    #endif
}

TEmbedBoard::TEmbedBoard()
    : displayConfig(createDisplayConfig()),
      deviceView(displayConfig) {}

void TEmbedBoard::initialize() {
    initializeBoardPeripherals();
    deviceView.initialize();
    deviceView.logo();
    deviceInput.waitPress(3000);
    deviceView.clear();
}

IDeviceView& TEmbedBoard::getDeviceView() {
    return deviceView;
}

IInput& TEmbedBoard::getDeviceInput() {
    return deviceInput;
}

IHostSerial& TEmbedBoard::getHostSerial() {
    return hostSerial;
}

#endif
