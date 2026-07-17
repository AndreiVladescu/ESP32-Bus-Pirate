#pragma once
#include <string>
#include <vector>
#include <freertos/FreeRTOS.h>
#include "Interfaces/IICMPService.h"

class ICMPService : public IICMPService {
public:
    ICMPService();
    ~ICMPService() override;

    // Normal ping
    void startPingTask(const std::string& host, int count = 5, int timeout_ms = 1000, int interval_ms = 200) override;
    // Discovery of devices
    void startDiscoveryTask(const std::string deviceIP, int timeout_ms = 200) override;
    static void discoveryTask(void* params);

    // Results
    bool isPingReady() const override { return pingReady; }
    ping_rc_t lastRc() const override { return pingRC; }
    int lastMedianMs() const override { return pingMedianMs; }
    int lastSent() const override { return pingTX; }
    int lastRecv() const override { return pingRX; }
    const std::string& getReport() const override { return report; }
    std::string getPingHelp() const override;
    bool isDiscoveryReady() const override { return discoveryReady; }

    // Task entry
    static void pingAPI(void *pvParams);

    // Responsive ICMP logging
    std::vector<std::string> fetchICMPLog() override;
    void clearICMPLogging() override;
    void stopICMPService() override;
    void clearDiscoveryFlag() override { discoveryReady = false; }

private:
    bool pingReady = false;
    int  pingMedianMs = -1;
    int  pingTX = 0;
    int  pingRX = 0;
    std::string report;
    ping_rc_t pingRC = ping_rc_t::ping_error;
    bool discoveryReady = false;

    // Log buffer thread safe
    static portMUX_TYPE icmpMux;
    static std::vector<std::string> icmpLog;
    static constexpr size_t ICMP_LOG_MAX = 200;
    static bool stopICMPFlag;

    static void pushICMPLog(const std::string& line);
    static bool getICMPServiceStatus();
    // Clears non-static variables
    void cleanupICMPService();
};
