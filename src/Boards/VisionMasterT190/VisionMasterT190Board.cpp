#ifdef DEVICE_VISION_MASTER_T190

#include "Boards/VisionMasterT190/VisionMasterT190Board.h"

St7789SpiConfig VisionMasterT190Board::createDisplayConfig() {
    St7789SpiConfig config;
    config.pinBacklight = 17;
    config.pinMiso = -1;
    config.pinMosi = 48;
    config.pinSclk = 38;
    config.pinCs = 39;
    config.pinDc = 47;
    config.pinReset = 40;
    config.pinPower = 7;
    config.spiHost = SPI3_HOST;
    config.useSharedSpi = false;

    config.panelWidth = 170;
    config.panelHeight = 320;
    config.memoryWidth = 240;
    config.memoryHeight = 320;
    config.offsetX = 35;
    config.offsetY = 0;
    config.powerActiveHigh = false;
    return config;
}

VisionMasterT190Board::VisionMasterT190Board()
    : displayConfig(createDisplayConfig()),
      deviceView(displayConfig) {}

void VisionMasterT190Board::initialize() {
    deviceView.initialize();
    deviceView.logo();
    deviceInput.waitPress(3000);
    deviceView.clear();
}

IDeviceView& VisionMasterT190Board::getDeviceView() {
    return deviceView;
}

IInput& VisionMasterT190Board::getDeviceInput() {
    return deviceInput;
}

IHostSerial& VisionMasterT190Board::getHostSerial() {
    return hostSerial;
}

#endif
