#pragma once

#ifdef DEVICE_TDISPLAYS3

#include "Boards/Common/Views/St7789ParallelDeviceView.h"
#include "Boards/TDisplayS3/TdisplayInput.h"
#include "Boards/Common/Serial/BoardHostSerial.h"

class TDisplayS3Board final {
public:
    TDisplayS3Board();

    void initialize();
    IDeviceView& getDeviceView();
    IInput& getDeviceInput();
    IHostSerial& getHostSerial();

private:
    static St7789ParallelConfig createDisplayConfig();

    BoardHostSerial hostSerial;
    St7789ParallelConfig displayConfig;
    St7789ParallelDeviceView deviceView;
    TdisplayInput deviceInput;
};

#endif
