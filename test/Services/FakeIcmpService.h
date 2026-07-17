#pragma once

#include <deque>
#include <string>
#include <vector>

#include "Interfaces/IICMPService.h"

class FakeIcmpService final : public IICMPService {
public:
    struct PingCall {
        std::string host;
        int count = 0;
        int timeoutMs = 0;
        int intervalMs = 0;
    };

    struct DiscoveryCall {
        std::string deviceIp;
        int timeoutMs = 0;
    };

    bool pingReady = true;
    bool discoveryReady = true;
    ping_rc_t rc = ping_ok;
    int medianMs = 12;
    int sent = 5;
    int recv = 5;
    std::string report = "ping report\n";
    std::string help = "ping help";
    std::deque<std::vector<std::string>> logBatches;
    std::vector<PingCall> pingCalls;
    std::vector<DiscoveryCall> discoveryCalls;
    uint32_t clearLogCalls = 0;
    uint32_t stopCalls = 0;
    uint32_t clearDiscoveryCalls = 0;

    void startPingTask(const std::string& host, int count = 5, int timeout_ms = 1000, int interval_ms = 200) override {
        pingCalls.push_back({host, count, timeout_ms, interval_ms});
    }

    void startDiscoveryTask(const std::string deviceIP, int timeout_ms = 200) override {
        discoveryCalls.push_back({deviceIP, timeout_ms});
    }

    bool isPingReady() const override { return pingReady; }
    ping_rc_t lastRc() const override { return rc; }
    int lastMedianMs() const override { return medianMs; }
    int lastSent() const override { return sent; }
    int lastRecv() const override { return recv; }
    const std::string& getReport() const override { return report; }
    std::string getPingHelp() const override { return help; }
    bool isDiscoveryReady() const override { return discoveryReady; }

    std::vector<std::string> fetchICMPLog() override {
        if (logBatches.empty()) return {};
        auto batch = logBatches.front();
        logBatches.pop_front();
        return batch;
    }

    void clearICMPLogging() override { ++clearLogCalls; }
    void stopICMPService() override { ++stopCalls; discoveryReady = true; }
    void clearDiscoveryFlag() override { ++clearDiscoveryCalls; discoveryReady = false; }
};

