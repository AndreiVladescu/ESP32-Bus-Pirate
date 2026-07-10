#pragma once

#if defined(DEVICE_TEMBEDS3) || defined(DEVICE_TEMBEDS3CC1101)

#include "Boards/Common/Views/St7789SpiDeviceView.h"
#include "Boards/TEmbed/TEmbedInput.h"
#include "Boards/Common/Serial/BoardHostSerial.h"

class TEmbedBoard final {
public:
    TEmbedBoard();

    void initialize();
    IDeviceView& getDeviceView();
    IInput& getDeviceInput();
    IHostSerial& getHostSerial();

private:
    static St7789SpiConfig createDisplayConfig();
    static void initializeBoardPeripherals();

    BoardHostSerial hostSerial;
    St7789SpiConfig displayConfig;
    St7789SpiDeviceView deviceView;
    TEmbedInput deviceInput;
};

#endif
