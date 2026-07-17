#pragma once

#include <vector>
#include "Interfaces/ITerminalView.h"
#include "Interfaces/IInput.h"
#include "Interfaces/IUtilityService.h"
#include "Managers/UserInputManager.h"
#include "Transformers/ArgTransformer.h"
#include "States/GlobalState.h"
#include "Interfaces/IOneWireService.h"
#include "Interfaces/IShell.h"

class IbuttonShell : public IShell {
public:
    IbuttonShell(ITerminalView& terminalView,
                 IInput& terminalInput,
                 IUtilityService& utilityService,
                 UserInputManager& userInputManager,
                 ArgTransformer& argTransformer,
                 IOneWireService& oneWireService);

    void run() override;

private:
    void cmdReadId();
    void cmdWriteId();
    void cmdCopyId();

    ITerminalView& terminalView;
    IInput& terminalInput;
    IUtilityService& utilityService;
    UserInputManager& userInputManager;
    ArgTransformer& argTransformer;
    IOneWireService& oneWireService;
    GlobalState& state = GlobalState::getInstance();
};
