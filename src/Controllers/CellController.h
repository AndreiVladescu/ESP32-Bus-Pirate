#pragma once

#include <string>

#include "Interfaces/ITerminalView.h"
#include "Interfaces/IInput.h"
#include "Interfaces/IUtilityService.h"
#include "Interfaces/ICellService.h"
#include "Models/TerminalCommand.h"
#include "Transformers/ArgTransformer.h"
#include "Transformers/AtTransformer.h"
#include "Managers/UserInputManager.h"
#include "States/GlobalState.h"
#include "Shells/HelpShell.h"
#include "Interfaces/IShell.h"

class CellController {
public:
    CellController(ITerminalView& view,
                   IInput& terminalInput,
                   IUtilityService& utilityService,
                   ICellService& service,
                   ArgTransformer& argTransformer,
                   AtTransformer& atTransformer,
                   UserInputManager& userInputManager,
                   HelpShell& helpShell,
                   IShell& cellCallShell,
                   IShell& cellSmsShell);
    
    // Entry point for cell command
    void handleCommand(const TerminalCommand& command);

    // Configure before use
    void ensureConfigured();

private:
    ITerminalView& terminalView;
    IInput& terminalInput;
    IUtilityService& utilityService;
    ICellService& cellService;
    ArgTransformer& argTransformer;
    AtTransformer& atTransformer;
    UserInputManager& userInputManager;
    HelpShell& helpShell;
    IShell& cellCallShell;
    IShell& cellSmsShell;
    GlobalState& state = GlobalState::getInstance();

    bool configured = false;
    
    // Configure RX/TX pins and baudrate
    void handleConfig();

    // Display modem info and status
    void handleModem();

    // Display SIM card info
    void handleSim();

    // Set modem mode (CFUN)
    void handleSetMode();

    // Unlock SIM if PIN protected
    void handleUnlock(const TerminalCommand& command);

    // Display network registration and signal info
    void handleNetwork();
    
    // SMS shellsfor listing/sending/deleting SMS
    void handleSms(const TerminalCommand& command);

    // Send USSD cmd
    void handleUssd(const TerminalCommand& command);

    // Call hell (dial, answer, hangup, list)
    void handleCall(const TerminalCommand& command);

    // List operators and selected operator
    void handleOperator();

    // Display phonebook storage info and entries
    void handlePhoneBook();

    // Show help
    void handleHelp();
};
