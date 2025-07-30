#pragma once

#include <WiFi.h>
#include <ESP32Ping.h>
#include <string>
#include <vector>
#include <sstream>

extern "C" {
  #include "esp_wifi.h"
  #include "esp_wifi_types.h"
}

struct WiFiNetwork {
    std::string ssid;
    int32_t rssi = 0;
    wifi_auth_mode_t encryption = WIFI_AUTH_OPEN;
    bool open = false;
    bool vulnerable = false;

    std::string bssid;
    int32_t channel = 0;
    bool hidden = false;
};

struct SniffedPacket {
    int8_t rssi;
    uint8_t channel;
    std::string mac;
    uint8_t type;
};

class WifiService {
public:

    enum class MacInterface {
        Station,
        AccessPoint
    };
    
    WifiService();

    // Connection
    bool connect(const std::string& ssid, const std::string& password, unsigned long timeoutMs = 14000);
    void disconnect();
    bool isConnected() const;

    // Utils
    std::string getLocalIP() const;
    std::string getCurrentIP() const;
    void setModeApSta();
    void setModeApOnly();
    std::string getMacAddressSta() const;
    std::string getMacAddressAp() const;
    std::string getApIp() const;
    std::string getLocalIp() const;
    void reset();
    
    // Access point
    bool startAccessPoint(const std::string& ssid, const std::string& password = "", int channel = 1, int maxConn = 4);
    
    // Ping
    int ping(const std::string& host);

    // Spoof MAC
    bool spoofMacAddress(const std::string& macStr, MacInterface which);
    static std::string formatMac(const uint8_t* mac);

    // Scan
    std::vector<std::string> scanNetworks();
    std::vector<WiFiNetwork> scanDetailedNetworks();
    std::vector<WiFiNetwork> getOpenNetworks(const std::vector<WiFiNetwork>& networks);
    std::vector<WiFiNetwork> getVulnerableNetworks(const std::vector<WiFiNetwork>& networks);
    bool isVulnerable(wifi_auth_mode_t encryption) const;
    std::string encryptionTypeToString(wifi_auth_mode_t encryption);

    // Sniffing passif
    void startPassiveSniffing();
    void stopPassiveSniffing();
    std::vector<std::string> getSniffLog();
    bool switchChannel(uint8_t channel);
    static std::string getFrameTypeSubtype(const uint8_t* payload, uint8_t& type, uint8_t& subtype);
    static std::string parseSsidFromPacket(const uint8_t* payload, int len, uint8_t type, uint8_t subtype);
    static std::string getFrameTypeName(uint8_t type, uint8_t subtype);
    static void extractTypeSubtype(const uint8_t* payload, uint8_t& type, uint8_t& subtype);

    // Deathentication attacks
    bool deauthApBySsid (const std::string& ssid, uint8_t bursts = 20, uint32_t sniffMs = 400);
    void deauthAttack(const uint8_t bssid[6], uint8_t channel, uint8_t bursts, uint32_t sniffMs);

    bool connected;
    static void snifferCallback(void* buf, wifi_promiscuous_pkt_type_t type);
    static std::vector<std::string> sniffLog;
    static portMUX_TYPE sniffMux;

    static portMUX_TYPE staMux;
    static std::vector<std::array<uint8_t, 6>> staList;
    static uint8_t apBSSID[6];
    static void clientSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type);

};