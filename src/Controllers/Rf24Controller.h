#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include "Interfaces/ITerminalView.h"
#include "Interfaces/IInput.h"
#include "Interfaces/IDeviceView.h"
#include "Interfaces/IUtilityService.h"
#include "Models/TerminalCommand.h"
#include "Transformers/ArgTransformer.h"
#include "Managers/UserInputManager.h"
#include "States/GlobalState.h"
#include "Interfaces/IRf24Service.h"
#include "Interfaces/IPinService.h"
#include "Data/Rf24Channels.h"
#include "Shells/HelpShell.h"

class Rf24Controller {
public:
    Rf24Controller(ITerminalView& terminalView,
                   IInput& terminalInput,
                   IDeviceView& deviceView,
                   IUtilityService& utilityService,
                   IRf24Service& rf24Service,
                   IPinService& pinService,
                   ArgTransformer& argTransformer,
                   UserInputManager& userInputManager,
                   HelpShell& helpShell)
    : terminalView(terminalView),
      terminalInput(terminalInput),
      deviceView(deviceView),
      utilityService(utilityService),
      rf24Service(rf24Service),
      pinService(pinService),
      argTransformer(argTransformer),
      userInputManager(userInputManager),
      helpShell(helpShell) {}

    // Entry point for rf24 commands
    void handleCommand(const TerminalCommand& cmd);

    // Ensure NRF24 is configured before use
    void ensureConfigured();

private:
    // Command handlers
    void handleConfig();
    void handleReceive();
    void handleSend();
    void handleScan();
    void handleJam();
    void handleSweep();
    void handleWaterfall();
    void handleSetChannel();
    void handleHelp();

private:
    ITerminalView& terminalView;
    IInput& terminalInput;
    IDeviceView& deviceView;
    IUtilityService& utilityService;
    IRf24Service& rf24Service;
    IPinService& pinService;
    ArgTransformer& argTransformer;
    UserInputManager& userInputManager;
    HelpShell& helpShell;
    GlobalState& state = GlobalState::getInstance();

    bool configured = false;
};
