#pragma once

#ifdef DEVICE_STICKS3

#include "Boards/Common/Views/M5DeviceView.h"
#include "Boards/StickS3/StickInput.h"
#include "Boards/Common/Serial/BoardHostSerial.h"

class StickS3Board final {
public:
    void initialize();
    IDeviceView& getDeviceView();
    IInput& getDeviceInput();
    IHostSerial& getHostSerial();

private:
    BoardHostSerial hostSerial;
    M5DeviceView deviceView;
    StickInput deviceInput;
};

#endif
