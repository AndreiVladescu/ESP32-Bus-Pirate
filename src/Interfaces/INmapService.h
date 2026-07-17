#pragma once

#include <string>
#include <vector>

#include "Data/NmapUtils.h"
#include "Interfaces/IICMPService.h"

class ArgTransformer;

class INmapService {
public:
    virtual ~INmapService() = default;

    virtual void startTask(int verbosity) = 0;
    virtual bool parseHosts(const std::string& hosts_arg) = 0;
    virtual bool parsePorts(const std::string& ports_arg) = 0;
    virtual const std::string getReport() = 0;
    virtual const bool isReady() = 0;
    virtual void clean() = 0;
    virtual NmapOptions parseNmapArgs(const std::vector<std::string>& tokens) = 0;
    virtual void setDefaultPorts(bool tcp) = 0;
    virtual void setArgTransformer(ArgTransformer& argTransformer) = 0;
    virtual void setICMPService(IICMPService* icmpService) = 0;
    virtual void setLayer4(bool layer4Protocol) = 0;
    virtual void setOptions(const NmapOptions& options) = 0;
    virtual std::string getHelpText() = 0;
};

