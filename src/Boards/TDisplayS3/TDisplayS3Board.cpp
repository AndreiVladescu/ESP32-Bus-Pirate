#ifdef DEVICE_TDISPLAYS3

#include "Boards/TDisplayS3/TDisplayS3Board.h"

St7789ParallelConfig TDisplayS3Board::createDisplayConfig() {
    St7789ParallelConfig config;

    config.pinBacklight = 38;
    config.pinPower = 46;

    config.pinWr = 8;
    config.pinRd = 9;
    config.pinDc = 7;
    config.pinD0 = 39;
    config.pinD1 = 40;
    config.pinD2 = 41;
    config.pinD3 = 42;
    config.pinD4 = 45;
    config.pinD5 = 46;
    config.pinD6 = 47;
    config.pinD7 = 48;

    config.pinCs = 6;
    config.pinReset = 5;
    config.pinBusy = -1;

    config.panelWidth = 170;
    config.panelHeight = 320;
    config.memoryWidth = 240;
    config.memoryHeight = 320;
    config.offsetX = 35;
    config.offsetY = 0;

    config.writeFrequency = 20000000;
    config.rotation = 3;
    config.invert = true;
    config.rgbOrder = false;
    config.dlen16Bit = false;
    config.powerActiveHigh = true;
    config.backlightActiveHigh = true;

    config.selectionHelpLine1 = "Short press top button to change terminal";
    config.selectionHelpLine2 = "Short press bottom button to accept";

    return config;
}

TDisplayS3Board::TDisplayS3Board()
    : displayConfig(createDisplayConfig()),
      deviceView(displayConfig) {}

void TDisplayS3Board::initialize() {
    deviceView.initialize();
    deviceView.logo();
    deviceInput.waitPress(3000);
    deviceView.clear();
}

IDeviceView& TDisplayS3Board::getDeviceView() {
    return deviceView;
}

IInput& TDisplayS3Board::getDeviceInput() {
    return deviceInput;
}

IHostSerial& TDisplayS3Board::getHostSerial() {
    return hostSerial;
}

#endif
