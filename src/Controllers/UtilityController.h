#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <sstream>
#include <algorithm>
#include "Models/TerminalCommand.h"
#include "Interfaces/ITerminalView.h"
#include "Interfaces/IInput.h"
#include "Interfaces/IDeviceView.h"
#include "Interfaces/IUtilityService.h"
#include "States/GlobalState.h"
#include "Enums/ModeEnum.h"
#include "Services/PinService.h"
#include "Services/I2sService.h"
#include "Managers/UserInputManager.h"
#include "Analyzers/PinAnalyzer.h"
#include "Managers/AliasManager.h"
#include "Transformers/ArgTransformer.h"
#include "Transformers/TerminalCommandTransformer.h"
#include "Shells/SysInfoShell.h"
#include "Shells/GuideShell.h"
#include "Shells/HelpShell.h"
#include "Shells/ProfileShell.h"

class UtilityController {
public:
    // Constructor
    UtilityController(
        ITerminalView& terminalView, 
        IDeviceView& deviceView, 
        IInput& terminalInput, 
        IUtilityService& utilityService,
        PinService& pinService,
        I2sService& i2sService,
        UserInputManager& userInputManager,
        PinAnalyzer& pinAnalyzer,
        AliasManager& aliasManager,
        ArgTransformer& argTransformer,
        TerminalCommandTransformer& terminalCommandTransformer,
        SysInfoShell& sysInfoShell,
        GuideShell& guideShell,
        HelpShell& helpShell,
        ProfileShell& profileShell
    );

    // Entry point for global utility commands
    void handleCommand(const TerminalCommand& cmd);

    // Process mode change command and return new mode
    ModeEnum handleModeChangeCommand(const TerminalCommand& cmd);

    // Logic Analyzer on device screen
    void handleLogicAnalyzer(const TerminalCommand& cmd);

    // Analogic plotter on device screen
    void handleAnalogic(const TerminalCommand& cmd);

    // Load and save pin profiles
    void handleProfile();

private:
    // Display help for utility commands
    void handleHelp();

    // Ask user to select a mode
    ModeEnum handleModeSelect();

    // Enable internal pull-up resistors
    void handleEnablePullups();

    // Disable internal pull-up resistors
    void handleDisablePullups();

    // System information
    void handleSystem();

    // Firmware guide
    void handleGuide();

    // Pin diagnostic with periodic report
    void handleWizard(const TerminalCommand& cmd);

    // Pin activity to audio
    void handleListen(const TerminalCommand& cmd);

    // Hexadecimal converter
    void handleHex(const TerminalCommand& cmd);
    
    // Delay command (used for cmd pipeline)
    void handleDelay(const TerminalCommand& cmd);

    // Alias command to create custom shortcuts for commands
    void handleAlias();

    ITerminalView& terminalView;
    IDeviceView& deviceView;
    IInput& terminalInput;
    IUtilityService& utilityService;
    PinService& pinService;
    I2sService& i2sService;
    UserInputManager& userInputManager;
    PinAnalyzer& pinAnalyzer;
    AliasManager& aliasManager;
    ArgTransformer& argTransformer;
    TerminalCommandTransformer& commandTransformer;
    SysInfoShell& sysInfoShell;
    GuideShell& guideShell;
    HelpShell& helpShell;
    ProfileShell& profileShell;
    GlobalState& state = GlobalState::getInstance();
};
