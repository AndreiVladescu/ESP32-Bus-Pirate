#pragma once

#include <cstddef>
#include <string>

#include "Interfaces/IInput.h"
#include "Interfaces/ITerminalView.h"
#include "Managers/CommandHistoryManager.h"

class CommandLineManager {
public:
    CommandLineManager(
        ITerminalView& terminalView,
        IInput& terminalInput,
        IInput& deviceInput,
        CommandHistoryManager& historyManager
    );

    void waitPress();
    std::string readCommand(const std::string& mode);

private:
    static constexpr size_t MAX_COMMAND_LENGTH = 512;

    ITerminalView& terminalView;
    IInput& terminalInput;
    IInput& deviceInput;
    CommandHistoryManager& historyManager;

    bool handleEscapeSequence(
        char c,
        std::string& inputLine,
        size_t& cursorIndex,
        const std::string& mode
    );
    bool handleCardputerEscapeSequence(
        char c,
        size_t& cursorIndex,
        std::string& inputLine,
        const std::string& mode
    );
    bool handleEnter(char c, const std::string& inputLine);
    bool handleTabCompletion(
        char c,
        std::string& inputLine,
        size_t& cursorIndex,
        const std::string& mode
    );
    bool handleBackspace(
        char c,
        std::string& inputLine,
        size_t& cursorIndex,
        const std::string& mode
    );
    bool handlePrintableChar(
        char c,
        std::string& inputLine,
        size_t& cursorIndex,
        const std::string& mode
    );
};
