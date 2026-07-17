#include "CellSmsShell.h"

CellSmsShell::CellSmsShell(ITerminalView& terminalView,
                           IInput& terminalInput,
                           UserInputManager& userInputManager,
                           ArgTransformer& argTransformer,
                           AtTransformer& atTransformer,
                           ICellService& cellService)
    : terminalView(terminalView),
      terminalInput(terminalInput),
      userInputManager(userInputManager),
      argTransformer(argTransformer),
      atTransformer(atTransformer),
      cellService(cellService)
{
}

void CellSmsShell::run()
{
    while (true) {
        terminalView.println("\n=== CELL SMS Shell ===");

        int idx = userInputManager.readValidatedChoiceIndex(
            "Select an action",
            ACTIONS,
            ACTION_COUNT,
            ACTION_COUNT - 1
        );

        if (idx < 0 || idx >= (int)ACTION_COUNT || idx == (int)ACTION_COUNT - 1) {
            terminalView.println("Exiting SMS Shell...\n");
            break;
        }

        switch (idx) {
            case 0: cmdList();   break;
            case 1: cmdRead();   break;
            case 2: cmdDelete(); break;
            case 3: cmdSend();   break;
            default:
                terminalView.println("❌ Unknown choice.");
                break;
        }
    }
}

void CellSmsShell::cmdList()
{
    // Filters are quite useless for now, just list all 
    // int idx = userInputManager.readValidatedChoiceIndex("Select filter", LIST_FILTERS, LIST_FILTER_COUNT, -1);
    // if (idx < 0 || idx >= (int)LIST_FILTER_COUNT || idx == (int)LIST_FILTER_COUNT - 1) {
    //     return; // Exit
    // }

    int idx = 0; // ALL for now

    cellService.smsSetTextMode(true);
    cellService.smsSetCharset("GSM");

    auto raw = cellService.smsList(LIST_FILTERS[idx]);

    terminalView.println("");
    terminalView.println(atTransformer.formatSmsList(raw));
}

void CellSmsShell::cmdRead()
{
    uint32_t idx = userInputManager.readValidatedUint32("SMS index", 0);

    cellService.smsSetTextMode(true);
    cellService.smsSetCharset("GSM");

    auto raw = cellService.smsRead((uint16_t)idx);

    terminalView.println("");
    terminalView.println(atTransformer.formatSmsRead(raw));
}

void CellSmsShell::cmdDelete()
{
    int f = userInputManager.readValidatedChoiceIndex("Delete flag", DELETE_FLAGS, DELETE_FLAG_COUNT, -1);
    if (f < 0 || f >= (int)DELETE_FLAG_COUNT || f == (int)DELETE_FLAG_COUNT - 1) {
        return; // Exit
    }

    uint16_t index = 0;
    std::string confirmMsg;

    // Flag 0 = delete one message
    if (f == 0) {
        index = (uint16_t)userInputManager.readValidatedUint32("SMS index", 0);
        confirmMsg = "Delete SMS at index " + std::to_string(index) + "?";
    }
    else {
        confirmMsg = "Confirm " + std::to_string(f) + " (" + DELETE_FLAGS[f] + ")?";
    }

    bool confirm = userInputManager.readYesNo(confirmMsg, false);
    if (!confirm) {
        terminalView.println("\n❌ SMS delete cancelled.");
        return;
    }

    bool ok = cellService.smsDelete(index, (uint8_t)f);
    terminalView.println(ok ? "\n✅ SMS delete: OK" : "\n❌ SMS delete: ERROR");
}

void CellSmsShell::cmdSend()
{
    std::string number = userInputManager.readValidatedPhoneNumber("Phone number (ex: +33601020304)", 6, 15);
    if (number.empty()) {
        terminalView.println("\n❌ Invalid phone number.");
        return;
    }
    std::string text   = userInputManager.readString("Message text", "Hello");

    cellService.smsSetTextMode(true);
    cellService.smsSetCharset("GSM");

    auto confirm = userInputManager.readYesNo("Send SMS to " + number + "?", true);
    if (!confirm) {
        terminalView.println("\n❌ SMS send cancelled.");
        return;
    }

    terminalView.println("Sending SMS... It may take 15 sec...");

    bool ready = cellService.smsBeginSend(number);
    if (!ready) {
        terminalView.println("\n❌ CMGS prompt not received (ERROR).");
        return;
    }

    bool ok = cellService.smsSendText(text);
    terminalView.println(ok ? "\n✅ SMS sent: OK" : "\n❌ SMS sent: ERROR");
}
