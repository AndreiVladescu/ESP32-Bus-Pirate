/**
 * @file CardWifiSetup.h
 * @author Aurélio Avanzi Dutch version Roland Breedveld adapted by Milton Matuda for Mic WebServer
 * @brief https://github.com/cyberwisk/M5Card_Wifi_KeyBoard_Setup/tree/main/M5Card_Wifi_KeyBoard_Setup
 * @version Apha 0.4BR
 * @date 2024-11-14
 *
 * @Hardwares: M5Cardputer - https://docs.m5stack.com/en/core/Cardputer
 * @Dependent Librarys:
 * M5Cardputer: https://github.com/m5stack/M5Cardputer
 * WiFi: https://github.com/espressif/arduino-esp32/tree/master/libraries/WiFi
 **/

#pragma once

#ifdef DEVICE_CARDPUTER

#include <WiFi.h>
#include <M5Cardputer.h>
#include <Preferences.h>

#define NVS_SSID_KEY "ssid"
#define NVS_PASS_KEY "pass"
#define NVS_DOMAIN "wifi_settings"

inline Preferences& getPreferences() {
    static Preferences preferences;
    return preferences;
}

String inputText(const String& prompt, int x, int y);
int scanWifiNetworks();
String selectWifiNetwork(int numNetworks);
void setWifiCredentials(String ssid, String password);
void getWifiCredentials(String& ssid, String& password);
bool connectToWifi(String wifiSSID, String wifiPassword);
String askWifiPassword(String ssid);
void setupCardputerWifi();

#endif