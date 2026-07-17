#pragma once

#include <cstdint>
#include <string>
#include <vector>

class IWifiOpenScannerService {
public:
    virtual ~IWifiOpenScannerService() = default;

    virtual bool startOpenProbe(uint32_t scanIntervalMs = 200) = 0;
    virtual void stopOpenProbe() = 0;
    virtual bool isOpenProbeRunning() const = 0;
    virtual std::vector<std::string> fetchProbeLog() = 0;
    virtual void clearProbeLog() = 0;
};

