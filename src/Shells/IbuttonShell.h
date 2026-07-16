#pragma once

#include <vector>
#include "Interfaces/ITerminalView.h"
#include "Interfaces/IInput.h"
#include "Interfaces/IUtilityService.h"
#include "Managers/UserInputManager.h"
#include "Transformers/ArgTransformer.h"
#include "States/GlobalState.h"
#include "Services/OneWireService.h"

class IbuttonShell {
public:
    IbuttonShell(ITerminalView& terminalView,
                 IInput& terminalInput,
                 IUtilityService& utilityService,
                 UserInputManager& userInputManager,
                 ArgTransformer& argTransformer,
                 OneWireService& oneWireService);

    void run();

private:
    void cmdReadId();
    void cmdWriteId();
    void cmdCopyId();

    ITerminalView& terminalView;
    IInput& terminalInput;
    IUtilityService& utilityService;
    UserInputManager& userInputManager;
    ArgTransformer& argTransformer;
    OneWireService& oneWireService;
    GlobalState& state = GlobalState::getInstance();
};
