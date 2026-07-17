#pragma once

#include "Interfaces/IInfraredService.h"
#include "Interfaces/ITerminalView.h"
#include "Interfaces/IInput.h"
#include "Interfaces/IUtilityService.h"
#include "Interfaces/IShell.h"
#include "Transformers/ArgTransformer.h"
#include "Managers/UserInputManager.h"
#include "Data/UniversalRemoteCommands.h"

class UniversalRemoteShell : public IShell {
public:
    UniversalRemoteShell(ITerminalView& view, IInput& input, IUtilityService& utilityService, IInfraredService& irService, ArgTransformer& argTransformer, UserInputManager& userInputManager);
    void run() override;

private:
    IInfraredService& infraredService;
    ITerminalView& terminalView;
    IInput& terminalInput;
    IUtilityService& utilityService;
    ArgTransformer& argTransformer;
    UserInputManager& userInputManager;

    void sendCommandGroup(const InfraredCommandStruct* group, size_t size);
};
