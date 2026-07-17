#pragma once

#include <string>
#include <vector>
#include "Transformers/ArgTransformer.h"
#include "Interfaces/INmapService.h"
#include "Data/NmapUtils.h"

class NmapService : public INmapService {
public:
    NmapService();
    void startTask(int verbosity) override;
    bool parseHosts(const std::string& hosts_arg) override;
    bool parsePorts(const std::string& ports_arg) override;
    const std::string getReport() override;
    const bool isReady() override;
    void clean() override;

    NmapOptions parseNmapArgs(const std::vector<std::string>& tokens) override;
    void setDefaultPorts(bool tcp) override;
    void setArgTransformer(ArgTransformer& argTransformer) override;
    void setICMPService(IICMPService* icmpService) override;
    void setLayer4(bool layer4Protocol) override;
    void setOptions(const NmapOptions& options) override;

    std::string getHelpText() override;

private:
    // Nmap Task, cause overflow if it runs in the main loop, so it must run in a dedicated FreeRTOS task with a larger stack
    static void scanTask(void *pvParams);
    bool isIpv4(const std::string& address);
    void scanTarget(const std::string &host, const std::vector<uint16_t> &ports);

    IICMPService* icmpService;
    std::vector<std::string> targetHosts;
    std::vector<uint16_t> targetPorts;
    bool ready;
    std::string report;
    Layer4Protocol layer4Protocol;
    ArgTransformer* argTransformer;
    int verbosity;

    NmapOptions _options;
};
