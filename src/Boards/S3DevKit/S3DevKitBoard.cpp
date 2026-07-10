#ifdef DEVICE_S3DEVKIT

#include "Boards/S3DevKit/S3DevKitBoard.h"

void S3DevKitBoard::initialize() {
    deviceView.initialize();
}

IDeviceView& S3DevKitBoard::getDeviceView() {
    return deviceView;
}

IInput& S3DevKitBoard::getDeviceInput() {
    return deviceInput;
}

IHostSerial& S3DevKitBoard::getHostSerial() {
    return hostSerial;
}

#endif
