#ifdef DEVICE_STICKS3

#include "Boards/StickS3/StickS3Board.h"
#include <M5Unified.h>

void StickS3Board::initialize() {
    auto cfg = M5.config();
    M5.begin(cfg);
    deviceView.setRotation(3);
    M5.Power.setExtOutput(false);
    deviceView.logo();
    deviceInput.waitPress(3000);
}

IDeviceView& StickS3Board::getDeviceView() {
    return deviceView;
}

IInput& StickS3Board::getDeviceInput() {
    return deviceInput;
}

IHostSerial& StickS3Board::getHostSerial() {
    return hostSerial;
}

#endif
