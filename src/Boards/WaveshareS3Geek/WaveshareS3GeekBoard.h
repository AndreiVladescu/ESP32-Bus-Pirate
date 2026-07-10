#pragma once

#ifdef DEVICE_WAVESHARE_S3_GEEK

#include "Boards/WaveshareS3Geek/WaveshareS3GeekDeviceView.h"
#include "Boards/WaveshareS3Geek/WaveshareS3GeekInput.h"
#include "Boards/Common/Serial/BoardHostSerial.h"

class WaveshareS3GeekBoard final {
public:
    void initialize();
    IDeviceView& getDeviceView();
    IInput& getDeviceInput();
    IHostSerial& getHostSerial();

private:
    BoardHostSerial hostSerial;
    WaveshareS3GeekDeviceView deviceView;
    WaveshareS3GeekInput deviceInput;
};

#endif
