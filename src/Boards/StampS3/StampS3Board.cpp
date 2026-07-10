#ifdef DEVICE_M5STAMPS3

#include "Boards/StampS3/StampS3Board.h"
#include <M5Unified.h>

void StampS3Board::initialize() {
    auto cfg = M5.config();
    M5.begin(cfg);
}

IDeviceView& StampS3Board::getDeviceView() {
    return deviceView;
}

IInput& StampS3Board::getDeviceInput() {
    return deviceInput;
}

IHostSerial& StampS3Board::getHostSerial() {
    return hostSerial;
}

#endif
