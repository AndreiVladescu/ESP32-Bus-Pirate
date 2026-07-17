#pragma once

#include <string>
#include <vector>

#include "Interfaces/INmapService.h"

class FakeNmapService final : public INmapService {
public:
    bool parseHostsResult = true;
    bool parsePortsResult = true;
    bool ready = true;
    std::string report = "nmap report";
    std::string help = "nmap help";
    std::vector<std::string> parsedHosts;
    std::vector<std::string> parsedPorts;
    std::vector<int> startVerbosity;
    std::vector<bool> defaultPortsTcp;
    std::vector<bool> layer4Values;
    std::vector<NmapOptions> optionCalls;
    ArgTransformer* argTransformer = nullptr;
    IICMPService* icmpService = nullptr;
    uint32_t cleanCalls = 0;

    void startTask(int verbosity) override { startVerbosity.push_back(verbosity); }

    bool parseHosts(const std::string& hosts_arg) override {
        parsedHosts.push_back(hosts_arg);
        return parseHostsResult;
    }

    bool parsePorts(const std::string& ports_arg) override {
        parsedPorts.push_back(ports_arg);
        return parsePortsResult;
    }

    const std::string getReport() override { return report; }
    const bool isReady() override { return ready; }
    void clean() override { ++cleanCalls; }

    NmapOptions parseNmapArgs(const std::vector<std::string>& tokens) override {
        NmapOptions options;
        for (size_t i = 0; i < tokens.size(); ++i) {
            if (tokens[i] == "-h" || tokens[i] == "--help") {
                options.help = true;
            } else if (tokens[i] == "-p" && i + 1 < tokens.size()) {
                options.hasPort = true;
                options.ports = tokens[++i];
            } else if (tokens[i] == "-sU") {
                options.tcp = false;
                options.udp = true;
            } else if (tokens[i] == "-sn") {
                options.pingOnly = true;
            } else if (tokens[i] == "-v") {
                ++options.verbosity;
            } else if (tokens[i] == "-vv") {
                options.verbosity += 2;
            }
        }
        return options;
    }

    void setDefaultPorts(bool tcp) override { defaultPortsTcp.push_back(tcp); }
    void setArgTransformer(ArgTransformer& transformer) override { argTransformer = &transformer; }
    void setICMPService(IICMPService* service) override { icmpService = service; }
    void setLayer4(bool layer4Protocol) override { layer4Values.push_back(layer4Protocol); }
    void setOptions(const NmapOptions& options) override { optionCalls.push_back(options); }
    std::string getHelpText() override { return help; }
};

