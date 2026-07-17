#pragma once
#include <string>
#include <vector>
#include "Interfaces/ITerminalView.h"
#include "Interfaces/IInput.h"
#include "Managers/UserInputManager.h"
#include "Interfaces/IThreeWireService.h"
#include "Transformers/ArgTransformer.h"
#include "States/GlobalState.h"
#include "Interfaces/IShell.h"

class ThreeWireEepromShell : public IShell {
public:
    ThreeWireEepromShell(
        ITerminalView& terminalView,
        IInput& terminalInput,
        UserInputManager& userInputManager,
        IThreeWireService& threeWireService,
        ArgTransformer& argTransformer);

    void run() override;

private:
    void cmdProbe();
    void cmdRead();
    void cmdWrite();
    void cmdDump();
    void cmdErase();


    ITerminalView& terminalView;
    IInput& terminalInput;
    UserInputManager& userInputManager;
    IThreeWireService& threeWireService;
    ArgTransformer& argTransformer;
    GlobalState& state = GlobalState::getInstance();
};
