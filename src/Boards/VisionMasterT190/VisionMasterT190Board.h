#pragma once

#ifdef DEVICE_VISION_MASTER_T190

#include "Boards/Common/Views/St7789SpiDeviceView.h"
#include "Boards/VisionMasterT190/VisionMasterT190Input.h"
#include "Boards/Common/Serial/BoardHostSerial.h"

class VisionMasterT190Board final {
public:
    VisionMasterT190Board();

    void initialize();
    IDeviceView& getDeviceView();
    IInput& getDeviceInput();
    IHostSerial& getHostSerial();

private:
    static St7789SpiConfig createDisplayConfig();

    BoardHostSerial hostSerial;
    St7789SpiConfig displayConfig;
    St7789SpiDeviceView deviceView;
    VisionMasterT190Input deviceInput;
};

#endif
