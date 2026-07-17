#pragma once
#include <string>
#include <vector>

#include "Interfaces/ITerminalView.h"
#include "Interfaces/IInput.h"
#include "Interfaces/IUtilityService.h"
#include "Managers/UserInputManager.h"
#include "Transformers/ArgTransformer.h"
#include "Transformers/AtTransformer.h"
#include "Interfaces/ICellService.h"
#include "Interfaces/IShell.h"

class ITerminalView;
class IInput;
class UserInputManager;
class ArgTransformer;
class AtTransformer;

class CellCallShell : public IShell {
public:
    CellCallShell(ITerminalView& terminalView,
                 IInput& terminalInput,
                 IUtilityService& utilityService,
                 UserInputManager& userInputManager,
                 ArgTransformer& argTransformer,
                 AtTransformer& atTransformer,
                 ICellService& cellService);

    void run() override;

private:
    void cmdDial();
    void cmdAnswer();
    void cmdHangup();
    void cmdList();

    ITerminalView& terminalView;
    IInput& terminalInput;
    IUtilityService& utilityService;
    UserInputManager& userInputManager;
    ArgTransformer& argTransformer;
    AtTransformer& atTransformer;
    ICellService& cellService;

    inline static const char* kActions[] = {
        " 📞 Dial number",
        " ✅ Answer incoming call",
        " 📴 Hang up",
        " 📋 List calls (CLCC)",
        " 🚪 Exit Shell"
    };

    inline static constexpr size_t kActionCount =
        sizeof(kActions) / sizeof(kActions[0]);
};
