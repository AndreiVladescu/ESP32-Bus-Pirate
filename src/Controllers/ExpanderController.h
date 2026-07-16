#pragma once

#include <string>
#include "Models/TerminalCommand.h"
#include "Services/UartService.h"
#include "Interfaces/ITerminalView.h"
#include "Interfaces/IInput.h"
#include "Interfaces/IUtilityService.h"
#include "Transformers/ArgTransformer.h"
#include "Managers/UserInputManager.h"
#include "Shells/HelpShell.h"
#include "States/GlobalState.h"

class ExpanderController {
public:
    // Constructor
    ExpanderController(ITerminalView& terminalView,
                 IInput& terminalInput,
                 IUtilityService& utilityService,
                 UartService& uartService,
                 ArgTransformer& argTransformer,
                 UserInputManager& userInputManager,
                 HelpShell& helpShell);
    
    // Entry point for Expander cmds
    void handleCommand(const TerminalCommand& cmd);

    // Ensure Expander configured before use
    void ensureConfigured();

private:
    // Configure the Expander
    void handleConfig();

    // Handle the UART bridge with the Expander
    void handleBridge();

    ITerminalView& terminalView;
    IInput& terminalInput;
    IUtilityService& utilityService;
    UartService& uartService;
    ArgTransformer& argTransformer;
    UserInputManager& userInputManager;
    HelpShell& helpShell;
    GlobalState& state = GlobalState::getInstance();

    // Fixed UART settings for the Expander
    const uint32_t baud = 115200;
    const uint8_t dataBits = 8;
    const char parityChar = 'N';
    const uint8_t stopBits = 1;
    const bool inverted = false;

    bool configured = false;
};
