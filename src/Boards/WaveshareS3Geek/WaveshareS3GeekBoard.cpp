#ifdef DEVICE_WAVESHARE_S3_GEEK

#include "Boards/WaveshareS3Geek/WaveshareS3GeekBoard.h"

void WaveshareS3GeekBoard::initialize() {
    deviceView.initialize();
    deviceView.logo();
    deviceInput.waitPress(3000);
    deviceView.clear();
}

IDeviceView& WaveshareS3GeekBoard::getDeviceView() {
    return deviceView;
}

IInput& WaveshareS3GeekBoard::getDeviceInput() {
    return deviceInput;
}

IHostSerial& WaveshareS3GeekBoard::getHostSerial() {
    return hostSerial;
}

#endif
