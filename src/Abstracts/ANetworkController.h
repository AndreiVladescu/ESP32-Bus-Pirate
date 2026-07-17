#pragma once

#include <cstdint>
#include <string>
#include "Interfaces/ITerminalView.h"
#include "Interfaces/IInput.h"
#include "Interfaces/IDeviceView.h"
#include "Interfaces/IUtilityService.h"
#include "Interfaces/IWifiService.h"
#include "Interfaces/IWifiOpenScannerService.h"
#include "Interfaces/IEthernetService.h"
#include "Interfaces/ISshService.h"
#include "Interfaces/INetcatService.h"
#include "Interfaces/INmapService.h"
#include "Interfaces/IICMPService.h"
#include "Interfaces/INvsService.h"
#include "Interfaces/ITelnetService.h"
#include "Interfaces/IHttpService.h"
#include "Interfaces/IJsonTransformer.h"
#include "Interfaces/IModbusShell.h"
#include "Transformers/ArgTransformer.h"
#include "Managers/UserInputManager.h"
#include "States/GlobalState.h"
#include "Models/TerminalCommand.h"
#include "Shells/HelpShell.h"

class ANetworkController {
public:

    ANetworkController(
        ITerminalView& terminalView,
        IDeviceView& deviceView,
        IInput& terminalInput,
        IInput& deviceInput,
        IUtilityService& utilityService,
        IWifiService& wifiService,
        IWifiOpenScannerService& wifiOpenNetworkService,
        IEthernetService& ethernetService,
        ISshService& sshService,
        INetcatService& netcatService,
        INmapService& nmapService,
        IICMPService& icmpService,
        INvsService& nvsService,
        IHttpService& httpService,
        ITelnetService& telnetService,
        ArgTransformer& argTransformer,
        IJsonTransformer& jsonTransformer,
        UserInputManager& userInputManager,
        IModbusShell& modbusShell,
        HelpShell& helpShell
    );

protected:
    void handleNetcat(const TerminalCommand& cmd);
    void handleNmap(const TerminalCommand& cmd);
    void handleSsh(const TerminalCommand& cmd);
    void handlePing(const TerminalCommand& cmd);
    void handleDiscovery(const TerminalCommand& cmd);
    void handleTelnet(const TerminalCommand& cmd);
    void handleModbus(const TerminalCommand& cmd);

    // HTTP
    void handleHttp(const TerminalCommand &cmd);
    void handleHttpGet(const TerminalCommand &cmd);
    void handleHttpAnalyze(const TerminalCommand &cmd);

    // Lookup
    void handleLookup(const TerminalCommand& cmd);
    void handleLookupMac(const TerminalCommand& cmd);
    void handleLookupIp(const TerminalCommand& cmd);


protected:
    ITerminalView&     terminalView;
    IDeviceView&       deviceView;
    IInput&            terminalInput;
    IInput&            deviceInput;
    IUtilityService&   utilityService;

    IWifiService&       wifiService;
    IEthernetService&   ethernetService;

    INvsService&        nvsService;

    IWifiOpenScannerService& wifiOpenScannerService;
    ISshService&        sshService;
    INetcatService&     netcatService;
    INmapService&       nmapService;
    IICMPService&       icmpService;
    IHttpService&       httpService;
    ITelnetService&     telnetService;

    IModbusShell&       modbusShell;
    HelpShell&        helpShell;

    ArgTransformer&    argTransformer;
    IJsonTransformer&   jsonTransformer;
    UserInputManager&  userInputManager;
    GlobalState&       globalState = GlobalState::getInstance();
};
