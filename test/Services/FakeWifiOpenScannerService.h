#pragma once

#include <deque>
#include <string>
#include <vector>

#include "Interfaces/IWifiOpenScannerService.h"

class FakeWifiOpenScannerService final : public IWifiOpenScannerService {
public:
    bool running = false;
    bool startResult = true;
    std::deque<std::vector<std::string>> logs;
    std::vector<uint32_t> startIntervals;
    uint32_t stopCalls = 0;
    uint32_t clearCalls = 0;

    bool startOpenProbe(uint32_t scanIntervalMs = 200) override {
        startIntervals.push_back(scanIntervalMs);
        running = startResult;
        return startResult;
    }

    void stopOpenProbe() override {
        ++stopCalls;
        running = false;
    }

    bool isOpenProbeRunning() const override { return running; }

    std::vector<std::string> fetchProbeLog() override {
        if (logs.empty()) return {};
        auto batch = logs.front();
        logs.pop_front();
        return batch;
    }

    void clearProbeLog() override { ++clearCalls; }
};

