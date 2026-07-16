#pragma once

#include "Services/InfraredService.h"
#include "Interfaces/ITerminalView.h"
#include "Interfaces/IInput.h"
#include "Interfaces/IUtilityService.h"
#include "Transformers/ArgTransformer.h"
#include "Managers/UserInputManager.h"
#include "Data/UniversalRemoteCommands.h"

class UniversalRemoteShell {
public:
    UniversalRemoteShell(ITerminalView& view, IInput& input, IUtilityService& utilityService, InfraredService& irService, ArgTransformer& argTransformer, UserInputManager& userInputManager);
    void run();

private:
    InfraredService& infraredService;
    ITerminalView& terminalView;
    IInput& terminalInput;
    IUtilityService& utilityService;
    ArgTransformer& argTransformer;
    UserInputManager& userInputManager;

    void sendCommandGroup(const InfraredCommandStruct* group, size_t size);
};
