#pragma once

#ifdef DEVICE_CARDPUTER

#include "Boards/Common/Views/M5DeviceView.h"
#include "Boards/Cardputer/CardputerInput.h"
#include "Boards/Common/Serial/BoardHostSerial.h"

class CardputerBoard final {
public:
    void initialize();
    IDeviceView& getDeviceView();
    IInput& getDeviceInput();
    IHostSerial& getHostSerial();

private:
    BoardHostSerial hostSerial;
    M5DeviceView deviceView;
    CardputerInput deviceInput;
};

#endif
