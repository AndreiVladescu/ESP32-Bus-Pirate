#pragma once

#if defined(DEVICE_TDISPLAYS3)

#include <Arduino.h>
#include "Data/InputKeys.h"
#include <Interfaces/IDeviceView.h>
//#include "Interfaces/IInput.h"

#define NVS_SSID_KEY "ssid"
#define NVS_PASS_KEY "pass"

#define DARK_GREY 0x4208
#define HELP_COLOR 0xC618

bool setupTdisplayWifi(IDeviceView& view);

#endif