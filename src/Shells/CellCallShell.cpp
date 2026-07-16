#include "CellCallShell.h"

CellCallShell::CellCallShell(ITerminalView& terminalView,
                             IInput& terminalInput,
                             IUtilityService& utilityService,
                             UserInputManager& userInputManager,
                             ArgTransformer& argTransformer,
                             AtTransformer& atTransformer,
                             CellService& cellService)
    : terminalView(terminalView),
      terminalInput(terminalInput),
      utilityService(utilityService),
      userInputManager(userInputManager),
      argTransformer(argTransformer),
      atTransformer(atTransformer),
      cellService(cellService)
{
}

void CellCallShell::run()
{
    while (true) {
        terminalView.println("\n=== CELL Call Shell ===");
        int idx = userInputManager.readValidatedChoiceIndex("Select an action", kActions, kActionCount, kActionCount - 1);

        if (idx < 0 || kActions[idx] == " 🚪 Exit Shell") {
            terminalView.println("Exiting Call Shell...\n");
            break;
        }

        switch (idx) {
            case 0: cmdDial();  break;
            case 1: cmdAnswer(); break;
            case 2: cmdHangup(); break;
            case 3: cmdList();   break;
            default:
                terminalView.println("❌ Unknown choice.\n");
                break;
        }
    }
}

void CellCallShell::cmdDial()
{
    std::string number = userInputManager.readValidatedPhoneNumber("Phone number (ex: +33601020304)");
    if (number.empty()) {
        terminalView.println("❌ Empty number.");
        return;
    }

    terminalView.println("Dialing " + number + " ...");
    bool ok = cellService.dial(number);
    terminalView.println(ok ? "\n✅ Dial: OK" : "\n❌ Dial: ERROR");

    if (!ok) return;

    terminalView.println("\nCall in progress... Press [ENTER] to hang up.\n");
    while (true) {
        int c = terminalInput.readChar();
        if (c == '\n' || c == '\r') {
            terminalView.println("⏹ Hanging up...");
            cellService.hangupCall();
            break;
        }
        
        utilityService.sleepMs(20);
    }
}

void CellCallShell::cmdAnswer()
{
    bool ok = cellService.answerCall();
    terminalView.println(ok ? "\n✅ Answer: OK" : "\n❌ Answer: ERROR");
}

void CellCallShell::cmdHangup()
{
    bool ok = cellService.hangupCall();
    terminalView.println(ok ? "\n✅ Hangup: OK" : "\n❌ Hangup: ERROR");
}

void CellCallShell::cmdList()
{
    auto raw = cellService.listCalls();
    terminalView.println(atTransformer.formatCallList(raw));
}
