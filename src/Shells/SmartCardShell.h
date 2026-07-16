
#pragma once

#include <vector>
#include <string>
#include "Services/TwoWireService.h"
#include "Managers/UserInputManager.h"
#include "Interfaces/IInput.h"
#include "Interfaces/ITerminalView.h"
#include "Interfaces/IUtilityService.h"

class SmartCardShell {
public:
    SmartCardShell(
        TwoWireService& twoWireService, 
        ITerminalView& terminalView, 
        IInput& terminalInput, 
        IUtilityService& utilityService,
        ArgTransformer& argTransformer,
        UserInputManager& userInputManager
    );
    void run();

private:
    TwoWireService& twoWireService;
    ITerminalView& terminalView;
    IInput& terminalInput;
    IUtilityService& utilityService;
    UserInputManager& userInputManager;
    ArgTransformer& argTransformer;

    void cmdProbe();
    void cmdSecurity();
    void cmdDump();
    void cmdUnlock();
    void cmdPsc(const std::string& subcommand);
    void cmdWrite();
    void cmdProtect();
};
