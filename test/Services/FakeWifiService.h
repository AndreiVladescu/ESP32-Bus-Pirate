#pragma once

#include <cstdint>
#include <deque>
#include <string>
#include <vector>

#include "Interfaces/IWifiService.h"

class FakeWifiService final : public IWifiService {
public:
    struct ConnectCall {
        std::string ssid;
        std::string password;
        unsigned long timeoutMs = 0;
    };

    struct AccessPointCall {
        std::string ssid;
        std::string password;
        int channel = 0;
        int maxConn = 0;
    };

    struct RepeaterCall {
        std::string staSsid;
        std::string staPass;
        std::string apSsid;
        std::string apPass;
        int apChannel = 0;
        int maxConn = 0;
        unsigned long timeoutMs = 0;
    };

    bool connected = false;
    bool connectResult = true;
    bool prepareRawTxResult = true;
    bool startAccessPointResult = true;
    bool stopAccessPointResult = true;
    bool spoofResult = true;
    bool repeaterRunning = false;
    bool startRepeaterResult = true;
    bool deauthResult = true;
    bool provisioningEnabled = false;

    int modeRaw = kWifiModeSta;
    int statusRaw = 6;
    int rssi = -50;
    int channel = 6;

    std::string localIp = "192.168.1.42";
    std::string currentIp = "192.168.1.42";
    std::string subnetMask = "255.255.255.0";
    std::string gatewayIp = "192.168.1.1";
    std::string dns1 = "1.1.1.1";
    std::string dns2 = "8.8.8.8";
    std::string hostname = "buspirate";
    std::string macSta = "02:AA:BB:CC:DD:EE";
    std::string macAp = "02:11:22:33:44:55";
    std::string apIp = "192.168.4.1";
    std::string ssid = "Lab WiFi";
    std::string bssid = "AA:BB:CC:DD:EE:FF";
    std::string repeaterIp = "192.168.5.1";

    std::vector<std::string> scanNetworksResult;
    std::vector<WiFiNetwork> detailedNetworks;
    std::deque<std::vector<std::string>> sniffLogBatches;
    std::vector<ConnectCall> connectCalls;
    std::vector<AccessPointCall> accessPointCalls;
    std::vector<RepeaterCall> repeaterCalls;
    std::vector<uint8_t> preparedRawChannels;
    std::vector<uint8_t> switchedChannels;
    std::vector<std::string> spoofedMacs;
    std::vector<WifiMacInterface> spoofedInterfaces;
    std::vector<std::string> deauthSsids;
    std::vector<uint8_t> rssiChannels;
    std::vector<uint8_t> packetChannels;
    std::vector<uint16_t> packetDwells;
    std::deque<int8_t> rssiSamples;
    std::deque<uint32_t> packetCounts;

    uint32_t disconnectCalls = 0;
    uint32_t resetCalls = 0;
    uint32_t recoverStaForRetryCalls = 0;
    uint32_t setModeApStaCalls = 0;
    uint32_t setModeApOnlyCalls = 0;
    uint32_t stopAccessPointCalls = 0;
    uint32_t startPassiveSniffingCalls = 0;
    uint32_t stopPassiveSniffingCalls = 0;
    uint32_t stopRepeaterCalls = 0;

    bool connect(const std::string& ssidValue,
                 const std::string& password,
                 unsigned long timeoutMs = 15000) override {
        connectCalls.push_back({ssidValue, password, timeoutMs});
        connected = connectResult;
        if (connected) {
            ssid = ssidValue;
            statusRaw = kWifiStatusConnected;
        }
        return connected;
    }

    void disconnect() override {
        ++disconnectCalls;
        connected = false;
        statusRaw = 6;
    }

    bool isConnected() const override { return connected; }
    std::string getLocalIP() const override { return localIp; }
    std::string getCurrentIP() const override { return currentIp; }
    std::string getSubnetMask() const override { return subnetMask; }
    std::string getGatewayIp() const override { return gatewayIp; }
    std::string getDns1() const override { return dns1; }
    std::string getDns2() const override { return dns2; }
    std::string getHostname() const override { return hostname; }
    void setModeApSta() override { ++setModeApStaCalls; modeRaw = kWifiModeApSta; }
    void setModeApOnly() override { ++setModeApOnlyCalls; modeRaw = kWifiModeAp; }
    std::string getMacAddressSta() const override { return macSta; }
    std::string getMacAddressAp() const override { return macAp; }
    std::string getApIp() const override { return apIp; }
    std::string getLocalIp() const override { return localIp; }
    int getRssi() const override { return rssi; }
    int getChannel() const override { return channel; }
    std::string getSsid() const override { return ssid; }
    std::string getBssid() const override { return bssid; }
    int getWifiModeRaw() const override { return modeRaw; }
    int getWifiStatusRaw() const override { return statusRaw; }
    bool isProvisioningEnabled() const override { return provisioningEnabled; }
    void reset() override { ++resetCalls; connected = false; statusRaw = 6; }
    void recoverStaForRetry(bool) override { ++recoverStaForRetryCalls; }

    bool prepareRawTx(uint8_t channelValue = 1) override {
        preparedRawChannels.push_back(channelValue);
        return prepareRawTxResult;
    }

    bool startAccessPoint(const std::string& ssidValue,
                          const std::string& password = "",
                          int channelValue = 1,
                          int maxConn = 4) override {
        accessPointCalls.push_back({ssidValue, password, channelValue, maxConn});
        return startAccessPointResult;
    }

    bool stopAccessPoint() override {
        ++stopAccessPointCalls;
        return stopAccessPointResult;
    }

    bool spoofMacAddress(const std::string& macStr, WifiMacInterface which) override {
        spoofedMacs.push_back(macStr);
        spoofedInterfaces.push_back(which);
        return spoofResult;
    }

    std::vector<std::string> scanNetworks() override { return scanNetworksResult; }
    std::vector<WiFiNetwork> scanDetailedNetworks() override { return detailedNetworks; }

    std::string encryptionTypeToString(int encryption) override {
        if (encryption == 0) return "OPEN";
        if (encryption == 1) return "WEP";
        if (encryption == 2) return "WPA";
        return "UNKNOWN";
    }

    int8_t scanRssiOnChannel(uint8_t channelValue) override {
        rssiChannels.push_back(channelValue);
        if (rssiSamples.empty()) return -127;
        const int8_t value = rssiSamples.front();
        rssiSamples.pop_front();
        return value;
    }

    uint32_t countPacketsOnChannel(uint8_t channelValue, uint16_t dwellMs) override {
        packetChannels.push_back(channelValue);
        packetDwells.push_back(dwellMs);
        if (packetCounts.empty()) return 0;
        const uint32_t value = packetCounts.front();
        packetCounts.pop_front();
        return value;
    }

    void startPassiveSniffing() override { ++startPassiveSniffingCalls; }
    void stopPassiveSniffing() override { ++stopPassiveSniffingCalls; }

    std::vector<std::string> getSniffLog() override {
        if (sniffLogBatches.empty()) return {};
        auto batch = sniffLogBatches.front();
        sniffLogBatches.pop_front();
        return batch;
    }

    bool switchChannel(uint8_t channelValue) override {
        switchedChannels.push_back(channelValue);
        return true;
    }

    bool deauthApBySsid(const std::string& target) override {
        deauthSsids.push_back(target);
        return deauthResult;
    }

    bool startRepeater(const std::string& staSsid,
                       const std::string& staPass,
                       const std::string& apSsid,
                       const std::string& apPass,
                       int apChannel = 1,
                       int maxConn = 10,
                       unsigned long timeoutMs = 15000) override {
        repeaterCalls.push_back({staSsid, staPass, apSsid, apPass, apChannel, maxConn, timeoutMs});
        repeaterRunning = startRepeaterResult;
        return startRepeaterResult;
    }

    void stopRepeater() override {
        ++stopRepeaterCalls;
        repeaterRunning = false;
    }

    bool isRepeaterRunning() const override { return repeaterRunning; }
    std::string getRepeaterIp() const override { return repeaterIp; }
};

