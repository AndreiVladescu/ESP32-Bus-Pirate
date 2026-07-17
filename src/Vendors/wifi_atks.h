#ifndef __WIFI_ATKS_H__
#define __WIFI_ATKS_H__

#if __has_include("Vendors/WiFi.h")
#include "Vendors/WiFi.h"
inline void beaconCreate(const char* = "", uint8_t = 0, int = false) {}
#else
#include <WiFi.h>

// Random SSID if no ssid provided
void beaconCreate(const char* ssid = "", uint8_t channel = 0, int spam=false);
#endif

#endif
