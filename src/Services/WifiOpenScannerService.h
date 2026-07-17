#pragma once
#include <vector>
#include <string>
#include <atomic>
#include <WiFi.h>
#include <HTTPClient.h>
#include "Interfaces/IWifiOpenScannerService.h"

class WifiOpenScannerService : public IWifiOpenScannerService {
public:
    WifiOpenScannerService() = default;

    // Start the open probe task
    bool startOpenProbe(uint32_t scanIntervalMs = 200) override;
    void stopOpenProbe() override;
    bool isOpenProbeRunning() const override { return openProbeRunning.load(); }

    // Fetch logs from the probe task
    std::vector<std::string> fetchProbeLog() override;
    void clearProbeLog() override;

private:
    // Task management
    static void openProbeTaskThunk(void* arg);
    void openProbeTask(uint32_t scanIntervalMs);

    // Scan and connect to open networks
    int  doScan(bool showHidden, unsigned long& outScanMs);
    void maybeRecoverFromFastScan(unsigned long scanMs);
    void processAllNetworks(int count);
    void processOneNetwork(int idx);
    bool connectToNetwork(const std::string& ssid, bool isOpen, unsigned long timeoutMs,
                            std::string& outIp, unsigned long& outElapsedMs);
    void safeDisconnect(unsigned delayMs = 0);

    // HTTP connectivity check
    bool performHttpCheck(int& outHttpCode, unsigned long& outHttpMs);
        
    // Helpers
    static bool isOpenAuth(int enc); // WIFI_AUTH_OPEN
    std::string getSsid(int idx) const;
    const char* encToStr(int enc) const;

    // Log buffer thread safe
    static portMUX_TYPE probeMux;
    static std::vector<std::string> probeLog;
    static constexpr size_t PROBE_LOG_MAX = 200;
    static void pushProbeLog(const std::string& line);

    // State
    std::atomic<bool> openProbeRunning{false};
    TaskHandle_t openProbeHandle = nullptr;
};
