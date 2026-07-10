#ifdef DEVICE_VISION_MASTER_T190

#include "Boards/VisionMasterT190/VisionMasterT190WifiSetup.h"

#include <Preferences.h>
#include <WiFi.h>
#include <LovyanGFX.hpp>

namespace {

constexpr const char* NVS_SSID_KEY = "ssid";
constexpr const char* NVS_PASS_KEY = "pass";

bool loadWifiCredentials(String& ssid, String& password) {
    Preferences preferences;
    preferences.begin("wifi_settings", true);
    ssid = preferences.getString(NVS_SSID_KEY, "");
    password = preferences.getString(NVS_PASS_KEY, "");
    preferences.end();
    return !ssid.isEmpty() && !password.isEmpty();
}

void showWifiMessage(IDeviceView& view, const char* title, const char* description, uint16_t accentColor) {
    auto* tft = static_cast<lgfx::LGFX_Device*>(view.getScreen());
    const int margin = 20;

    tft->fillScreen(TFT_BLACK);
    tft->fillRoundRect(margin, margin, tft->width() - margin * 2, tft->height() - margin * 2, 5, 0x0841);
    tft->drawRoundRect(margin, margin, tft->width() - margin * 2, tft->height() - margin * 2, 5, accentColor);
    tft->setTextDatum(MC_DATUM);
    tft->setTextFont(2);
    tft->setTextSize(2);
    tft->setTextColor(accentColor, 0x0841);
    tft->drawString(title, tft->width() / 2, tft->height() / 2 - 18);
    tft->setTextSize(1);
    tft->setTextColor(TFT_LIGHTGRAY, 0x0841);
    tft->drawString(description, tft->width() / 2, tft->height() / 2 + 18);
    tft->setTextDatum(TL_DATUM);
}

}

bool setupVisionMasterT190Wifi(IDeviceView& view) {
    String ssid;
    String password;

    if (!loadWifiCredentials(ssid, password)) {
        showWifiMessage(view, "No saved WiFi", "USB Serial to setup", TFT_RED);
        delay(5000);
        return false;
    }

    showWifiMessage(view, "Connecting", ssid.c_str(), TFT_GREEN);
    WiFi.begin(ssid.c_str(), password.c_str());

    for (int i = 0; i < 30; ++i) {
        if (WiFi.status() == WL_CONNECTED) {
            return true;
        }
        delay(800);
    }

    showWifiMessage(view, "Failed to connect", "USB Serial to setup", TFT_RED);
    delay(4000);
    return false;
}

#endif
