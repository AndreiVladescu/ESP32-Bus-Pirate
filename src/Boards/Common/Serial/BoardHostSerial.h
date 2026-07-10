#pragma once

#if defined(DEVICE_HOST_SERIAL_UART)
#include <Boards/Common/Serial/UartHostSerial.h>
using BoardHostSerial = UartHostSerial;
#else
#include <Boards/Common/Serial/DefaultHostSerial.h>
using BoardHostSerial = DefaultHostSerial;
#endif
