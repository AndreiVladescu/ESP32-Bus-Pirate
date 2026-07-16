#pragma once
#include <string>
#include "Models/TerminalCommand.h"
#include "Interfaces/ITerminalView.h"
#include "Interfaces/IInput.h"
#include "Interfaces/IDeviceView.h"
#include "Interfaces/IUtilityService.h"
#include "Services/FmService.h"
#include "Transformers/ArgTransformer.h"
#include "Managers/UserInputManager.h"
#include "Shells/HelpShell.h"
#include "Shells/FmBroadcastShell.h"
#include "States/GlobalState.h"

class FmController {
public:
    FmController(
        ITerminalView& terminalView,
        IInput& terminalInput,
        IDeviceView& deviceView,
        IUtilityService& utilityService,
        FmService& fmService,
        ArgTransformer& argTransformer,
        UserInputManager& userInputManager,
        HelpShell& helpShell,
        FmBroadcastShell& fmBroadcastShell
    );

    // Entry point for fm command
    void handleCommand(const TerminalCommand& cmd);

    void ensureConfigured(); 

private:
    // begin + check module presence
    bool ensurePresent_();

    // Commands
    void handleConfig();
    void handleSweep();
    void handleTrace(const TerminalCommand& cmd);
    void handleWaterfall();
    void handleBroadcast();
    void handleReset();
    void handleHelp();

private:
    ITerminalView& terminalView;
    IInput& terminalInput;
    IDeviceView& deviceView;
    IUtilityService& utilityService;
    FmService& fmService;
    ArgTransformer& argTransformer;
    UserInputManager& userInputManager;
    HelpShell& helpShell;
    FmBroadcastShell& fmBroadcastShell;
    GlobalState& state = GlobalState::getInstance();

    bool configured = false;
};
