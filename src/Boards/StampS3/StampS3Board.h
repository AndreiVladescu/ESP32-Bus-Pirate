#pragma once

#ifdef DEVICE_M5STAMPS3

#include "Boards/Common/Views/NoScreenDeviceView.h"
#include "Boards/StampS3/StampS3Input.h"
#include "Boards/Common/Serial/BoardHostSerial.h"

class StampS3Board final {
public:
    void initialize();
    IDeviceView& getDeviceView();
    IInput& getDeviceInput();
    IHostSerial& getHostSerial();

private:
    BoardHostSerial hostSerial;
    NoScreenDeviceView deviceView;
    StampS3Input deviceInput;
};

#endif
