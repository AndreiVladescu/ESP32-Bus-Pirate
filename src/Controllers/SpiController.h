#pragma once

#include <vector>
#include "Interfaces/ITerminalView.h"
#include "Interfaces/IUtilityService.h"
#include "Interfaces/ISpiService.h"
#include "Interfaces/ISdService.h"
#include "Interfaces/IInput.h"
#include "Models/TerminalCommand.h"
#include "Models/ByteCode.h"
#include "Transformers/ArgTransformer.h"
#include "Managers/UserInputManager.h"
#include "Interfaces/IShell.h"
#include "Analyzers/BinaryAnalyzer.h"
#include "States/GlobalState.h"
#include "Data/FlashDatabase.h"
#include "Shells/HelpShell.h"

class SpiController {
public:
    // Constructor
    SpiController(
        ITerminalView& terminalView,
        IInput& terminalInput, IUtilityService& utilityService, ISpiService&
        spiService, ISdService& sdService,
        ArgTransformer& argTransformer,
        UserInputManager& userInputManager,
        BinaryAnalyzer& binaryAnalyzer,
        IShell& sdCardShell,
        IShell& spiFlashShell,
        IShell& spiEepromShell,
        HelpShell& helpShell
    );

    // Entry point for handle raw user command
    void handleCommand(const TerminalCommand& cmd);

    // Entry point for handle parsed instruction bytecodes
    void handleInstruction(const std::vector<ByteCode>& bytecodes);

    // Ensure SPI is properly configured before use
    void ensureConfigured();

private:
    ITerminalView& terminalView;
    IInput& terminalInput;
    IUtilityService& utilityService;
    ISpiService& spiService;
    ISdService& sdService;
    ArgTransformer& argTransformer;
    UserInputManager& userInputManager;
    BinaryAnalyzer& binaryAnalyzer;
    IShell& sdCardShell;
    IShell& spiFlashShell;
    IShell& spiEepromShell;
    HelpShell& helpShell;
    GlobalState& state = GlobalState::getInstance();
    bool configured = false;

    // Passive bus monitor
    void handleSniff();

    // Handle SPI flash operations
    void handleFlash(const TerminalCommand& cmd);

    // Handle EEPROM operations
    void handleEeprom(const TerminalCommand& cmd);

    // SD operations
    void handleSdCard();

    // Slave mode
    void handleSlave();

    // Configure SPI bus parameters
    void handleConfig();

    // Available commands
    void handleHelp();
};
