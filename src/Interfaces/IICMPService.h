#pragma once

#include <string>
#include <vector>

enum phy_interface_t {
    phy_none,
    phy_wifi,
    phy_eth
};

enum ping_rc_t {
    ping_ok,
    ping_timeout,
    ping_resolve_fail,
    ping_session_fail,
    ping_error
};

class IICMPService {
public:
    virtual ~IICMPService() = default;

    virtual void startPingTask(const std::string& host, int count = 5, int timeout_ms = 1000, int interval_ms = 200) = 0;
    virtual void startDiscoveryTask(const std::string deviceIP, int timeout_ms = 200) = 0;
    virtual bool isPingReady() const = 0;
    virtual ping_rc_t lastRc() const = 0;
    virtual int lastMedianMs() const = 0;
    virtual int lastSent() const = 0;
    virtual int lastRecv() const = 0;
    virtual const std::string& getReport() const = 0;
    virtual std::string getPingHelp() const = 0;
    virtual bool isDiscoveryReady() const = 0;
    virtual std::vector<std::string> fetchICMPLog() = 0;
    virtual void clearICMPLogging() = 0;
    virtual void stopICMPService() = 0;
    virtual void clearDiscoveryFlag() = 0;
};

