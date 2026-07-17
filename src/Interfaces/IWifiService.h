#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct WiFiNetwork {
    std::string ssid;
    int32_t rssi = 0;
    int encryption = 0;
    bool open = false;
    bool vulnerable = false;

    std::string bssid;
    int32_t channel = 0;
    bool hidden = false;
};

enum class WifiMacInterface {
    Station,
    AccessPoint
};

class IWifiService {
public:
    virtual ~IWifiService() = default;

    static constexpr int kWifiModeNull = 0;
    static constexpr int kWifiModeSta = 1;
    static constexpr int kWifiModeAp = 2;
    static constexpr int kWifiModeApSta = 3;
    static constexpr int kWifiStatusConnected = 3;

    virtual bool connect(const std::string& ssid, const std::string& password, unsigned long timeoutMs = 15000) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;

    virtual std::string getLocalIP() const = 0;
    virtual std::string getCurrentIP() const = 0;
    virtual std::string getSubnetMask() const = 0;
    virtual std::string getGatewayIp() const = 0;
    virtual std::string getDns1() const = 0;
    virtual std::string getDns2() const = 0;
    virtual std::string getHostname() const = 0;
    virtual void setModeApSta() = 0;
    virtual void setModeApOnly() = 0;
    virtual std::string getMacAddressSta() const = 0;
    virtual std::string getMacAddressAp() const = 0;
    virtual std::string getApIp() const = 0;
    virtual std::string getLocalIp() const = 0;
    virtual int getRssi() const = 0;
    virtual int getChannel() const = 0;
    virtual std::string getSsid() const = 0;
    virtual std::string getBssid() const = 0;
    virtual int getWifiModeRaw() const = 0;
    virtual int getWifiStatusRaw() const = 0;
    virtual bool isProvisioningEnabled() const = 0;
    virtual void reset() = 0;
    virtual void recoverStaForRetry(bool keepApMode) = 0;
    virtual bool prepareRawTx(uint8_t channel = 1) = 0;

    virtual bool startAccessPoint(const std::string& ssid, const std::string& password = "", int channel = 1, int maxConn = 4) = 0;
    virtual bool stopAccessPoint() = 0;
    virtual bool spoofMacAddress(const std::string& macStr, WifiMacInterface which) = 0;

    virtual std::vector<std::string> scanNetworks() = 0;
    virtual std::vector<WiFiNetwork> scanDetailedNetworks() = 0;
    virtual std::string encryptionTypeToString(int encryption) = 0;
    virtual int8_t scanRssiOnChannel(uint8_t channel) = 0;
    virtual uint32_t countPacketsOnChannel(uint8_t channel, uint16_t dwellMs) = 0;

    virtual void startPassiveSniffing() = 0;
    virtual void stopPassiveSniffing() = 0;
    virtual std::vector<std::string> getSniffLog() = 0;
    virtual bool switchChannel(uint8_t channel) = 0;
    virtual bool deauthApBySsid(const std::string& ssid) = 0;

    virtual bool startRepeater(const std::string& staSsid,
                               const std::string& staPass,
                               const std::string& apSsid,
                               const std::string& apPass,
                               int apChannel = 1,
                               int maxConn = 10,
                               unsigned long timeoutMs = 15000) = 0;
    virtual void stopRepeater() = 0;
    virtual bool isRepeaterRunning() const = 0;
    virtual std::string getRepeaterIp() const = 0;

    static inline const char* wifiModeToStr(int mode) {
        switch (mode) {
            case kWifiModeNull:  return "NULL";
            case kWifiModeSta:   return "STA";
            case kWifiModeAp:    return "AP";
            case kWifiModeApSta: return "AP+STA";
            default:             return "?";
        }
    }

    static inline const char* wlStatusToStr(int status) {
        switch (status) {
            case 0: return "IDLE";
            case 1: return "NO_SSID";
            case 2: return "SCAN_DONE";
            case 3: return "CONNECTED";
            case 4: return "CONNECT_FAILED";
            case 5: return "CONNECTION_LOST";
            case 6: return "DISCONNECTED";
            case 255: return "NO_SHIELD";
            default: return "?";
        }
    }
};

