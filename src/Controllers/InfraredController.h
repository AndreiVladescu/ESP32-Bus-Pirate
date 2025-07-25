// InfraredController.h
#pragma once

#include <sstream>
#include <string>
#include <algorithm>
#include <Arduino.h>

#include "Interfaces/ITerminalView.h"
#include "Interfaces/IInput.h"
#include "Services/InfraredService.h"
#include "Models/TerminalCommand.h"
#include "Transformers/ArgTransformer.h"
#include "Managers/UserInputManager.h"
#include "States/GlobalState.h"

class InfraredController {
public:
    // Constructor
    InfraredController(ITerminalView& view, IInput& terminalInput, 
                       InfraredService& service, ArgTransformer& ArgTransformer,
                       UserInputManager& userInputManager);

    // Entry point for Infraredcommand dispatch
    void handleCommand(const TerminalCommand& command);

    // Ensure infrared is properly configured
    void ensureConfigured();

private:
    ITerminalView& terminalView;
    IInput& terminalInput;
    InfraredService& infraredService;
    GlobalState& state = GlobalState::getInstance();
    ArgTransformer& argTransformer;
    UserInputManager& userInputManager;
    bool configured = false;

    // Configure IR settings
    void handleConfig();

    // Send IR command
    void handleSend(const TerminalCommand& command);

    // Receive IR commands
    void handleReceive();

    // Send "device-b-gone" style power-off signals
    void handleDeviceBgone();

    // Set IR protocol
    void handleSetProtocol();

    // Show help text
    void handleHelp();
};
