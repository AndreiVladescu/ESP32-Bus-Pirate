
#pragma once

#include <vector>
#include <string>
#include "Interfaces/ITwoWireService.h"
#include "Managers/UserInputManager.h"
#include "Interfaces/IInput.h"
#include "Interfaces/ITerminalView.h"
#include "Interfaces/IUtilityService.h"
#include "Interfaces/IShell.h"

class SmartCardShell : public IShell {
public:
    SmartCardShell(
        ITwoWireService& twoWireService,
        ITerminalView& terminalView,
        IInput& terminalInput,
        IUtilityService& utilityService,
        ArgTransformer& argTransformer,
        UserInputManager& userInputManager
    );
    void run() override;

private:
    ITwoWireService& twoWireService;
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
