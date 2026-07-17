#include "CommandLineManager.h"

#include <cctype>

#include "Data/AutoCompleteWords.h"
#include "Data/InputKeys.h"
#include "States/GlobalState.h"

CommandLineManager::CommandLineManager(
    ITerminalView& terminalView,
    IInput& terminalInput,
    IInput& deviceInput,
    CommandHistoryManager& historyManager
)
    : terminalView(terminalView),
      terminalInput(terminalInput),
      deviceInput(deviceInput),
      historyManager(historyManager) {}

void CommandLineManager::waitPress() {
    terminalInput.waitPress();
}

std::string CommandLineManager::readCommand(const std::string& mode) {
    std::string inputLine;
    size_t cursorIndex = 0;

    while (true) {
        deviceInput.readChar(); // to check shutdown request for T-Embeds
        char c = terminalInput.readChar();
        if (c == KEY_NONE) continue;

        if (handleCardputerEscapeSequence(c, cursorIndex, inputLine, mode)) continue;
        if (handleEscapeSequence(c, inputLine, cursorIndex, mode)) continue;
        if (handleEnter(c, inputLine)) return inputLine;
        if (handleBackspace(c, inputLine, cursorIndex, mode)) continue;
        if (handlePrintableChar(c, inputLine, cursorIndex, mode));
        if (handleTabCompletion(c, inputLine, cursorIndex, mode));
    }
}

bool CommandLineManager::handleCardputerEscapeSequence(
    char c,
    size_t& cursorIndex,
    std::string& inputLine,
    const std::string& mode
) {
    if (GlobalState::getInstance().getTerminalMode() != TerminalTypeEnum::Standalone) {
        return false;
    }

    if (c == CARDPUTER_SPECIAL_ARROW_UP) {
        terminalView.print(std::string(1, CARDPUTER_SPECIAL_ARROW_UP));
    } else if (c == CARDPUTER_SPECIAL_ARROW_DOWN) {
        terminalView.print(std::string(1, CARDPUTER_SPECIAL_ARROW_DOWN));
    } else if (c == '\t') {
        if (c != '\t') return false;

        // Clear current line
        inputLine.clear();

        // Recall previous command from history
        inputLine = historyManager.up();

        // Move cursor to end and redraw
        cursorIndex = inputLine.length();
        terminalView.print("\r" + mode + "> " + inputLine + "\033[K");

        return true;
    } else {
        return false;
    }

    return true;
}

bool CommandLineManager::handleEscapeSequence(
    char c,
    std::string& inputLine,
    size_t& cursorIndex,
    const std::string& mode
) {
    if (c != '\x1B') return false;

    if (terminalInput.readChar() == '[') {
        char next = terminalInput.readChar();

        if (next == 'A') {
            inputLine = historyManager.up();
            cursorIndex = inputLine.length();
        } else if (next == 'B') {
            inputLine = historyManager.down();
            cursorIndex = inputLine.length();
        } else if (next == 'C') {
            if (cursorIndex < inputLine.length()) {
                cursorIndex++;
                terminalView.print("\x1B[C");
            }
            return true;
        } else if (next == 'D') {
            if (cursorIndex > 0) {
                cursorIndex--;
                terminalView.print("\x1B[D");
            }
            return true;
        } else {
            return false;
        }

        terminalView.print("\r" + mode + "> " + inputLine + "\033[K");
        return true;
    }

    return false;
}

bool CommandLineManager::handleEnter(char c, const std::string& inputLine) {
    if (c != '\r' && c != '\n') return false;

    terminalView.println("");
    historyManager.add(inputLine);
    return true;
}

bool CommandLineManager::handleTabCompletion(
    char c,
    std::string& inputLine,
    size_t& cursorIndex,
    const std::string& mode
) {
    if (c != '\t') return false;

    // prefix up to cursor
    std::string prefix = inputLine.substr(0, cursorIndex);

    // history autocomplete
    std::string suggestion = historyManager.autocomplete(prefix);

    // fallback dictionary autocomplete
    if (suggestion.empty()) {
        for (size_t i = 0; autoCompleteWords[i] != nullptr; ++i) {
            std::string w(autoCompleteWords[i]);

            if (w.size() >= prefix.size() &&
                w.compare(0, prefix.size(), prefix) == 0) {
                suggestion = w;
                break;
            }
        }
    }

    // apply suggestion
    if (!suggestion.empty()) {
        inputLine = suggestion;
        cursorIndex = inputLine.length();
        terminalView.print("\r" + mode + "> " + inputLine + "\033[K");
    }

    return true;
}

bool CommandLineManager::handleBackspace(
    char c,
    std::string& inputLine,
    size_t& cursorIndex,
    const std::string& mode
) {
    if (c != '\b' && c != 127) return false;
    if (cursorIndex == 0) return true;

    cursorIndex--;
    inputLine.erase(cursorIndex, 1);

    terminalView.print("\r" + mode + "> " + inputLine + " \033[K");

    int moveBack = inputLine.length() - cursorIndex;
    for (int i = 0; i <= moveBack; ++i) {
        terminalView.print("\x1B[D");
    }

    return true;
}

bool CommandLineManager::handlePrintableChar(
    char c,
    std::string& inputLine,
    size_t& cursorIndex,
    const std::string& mode
) {
    if (!std::isprint(static_cast<unsigned char>(c))) return false;

    if (inputLine.size() >= MAX_COMMAND_LENGTH) {
        return true; // ignore extra chars
    }

    inputLine.insert(cursorIndex, 1, c);
    cursorIndex++;

    terminalView.print("\r" + mode + "> " + inputLine + "\033[K");

    size_t moveBack = inputLine.length() - cursorIndex;
    for (size_t i = 0; i < moveBack; ++i) {
        terminalView.print("\x1B[D");
    }

    return true;
}
