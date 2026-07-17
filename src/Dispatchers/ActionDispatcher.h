#pragma once

#include <string>
#include <vector>
#include "Models/TerminalCommand.h"
#include "Models/ByteCode.h"
#include "Transformers/InstructionTransformer.h"
#include "Enums/ModeEnum.h"
#include "Providers/DependencyProvider.h"
#include "Enums/ByteCodeEnum.h"
#include "Enums/TerminalTypeEnum.h"
#include "Interfaces/ITerminalView.h"

class ActionDispatcher {
public:
    // Constructor with dependency injection
    explicit ActionDispatcher(DependencyProvider& provider);

    // Initialize
    void setup(TerminalTypeEnum terminalType, std::string terminaInfos);

    // Main loop that handles user input
    void run();

    // Process raw user action
    void dispatch(const std::string& raw);

private:
    DependencyProvider& provider;
    GlobalState& state = GlobalState::getInstance();

    // Handle a command
    void dispatchCommand(const TerminalCommand& cmd);

    // dipatch repeat command
    void dispatchRepeatCommands(const std::string& raw);

    // dispatch a sequence of commands (pipeline)
    void dispatchPipelineCommands(const std::string& raw);

    // Handle a sequence of bytecode instructions
    void dispatchInstructions(const std::vector<Instruction>& instructions);

    // Switch to a different mode
    void setCurrentMode(ModeEnum newMode);

    // Release mode resources if needed
    void releaseMode(ModeEnum currentMode, ModeEnum newMode);
};
