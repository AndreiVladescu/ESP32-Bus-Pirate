#ifdef DEVICE_CARDPUTER

#include "Boards/Cardputer/CardputerBoard.h"
#include <M5Unified.h>
#include <M5Cardputer.h>

void CardputerBoard::initialize() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    deviceView.setRotation(1);
    deviceView.logo();
    deviceInput.waitPress(3000);
}

IDeviceView& CardputerBoard::getDeviceView() {
    return deviceView;
}

IInput& CardputerBoard::getDeviceInput() {
    return deviceInput;
}

IHostSerial& CardputerBoard::getHostSerial() {
    return hostSerial;
}

#endif
