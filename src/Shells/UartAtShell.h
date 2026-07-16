#pragma once
#include <vector>
#include <string>
#include "Interfaces/ITerminalView.h"
#include "Interfaces/IInput.h"
#include "Interfaces/IUtilityService.h"
#include "Managers/UserInputManager.h"
#include "Transformers/ArgTransformer.h"
#include "Services/UartService.h"
#include "Data/UartAtCommands.h"

class UartAtShell {
public:
    UartAtShell(ITerminalView& terminalView,
                IInput& terminalInput,
                IUtilityService& utilityService,
                UserInputManager& userInputManager,
                ArgTransformer& argTransformer,
                UartService& uartService);

    void run();

private:
    bool selectMode(AtMode& outMode);
    void actionLoop(AtMode& outMode);
    bool selectAction(AtActionSlice actions, const AtActionItem*& outAction);
    bool buildCommandFromArgs(const char* commandTemplate,
                              const AtActionArg* args,
                              std::size_t argCount,
                              std::string& outCmd);
    std::string readUserLine(const std::string& prompt);
    bool isInChoices(const std::string& v, const char* choices) const;
    bool validateAndFormat(const AtActionArg& a, const std::string& raw, std::string& out) const;
    bool acquireArgValue(const AtActionArg& a, size_t idx, std::string& accepted, bool& hasValue);
    void applyArgToCommand(std::string& cmd, size_t idx, const std::string& accepted, bool hasValue) const;
    std::string placeholderFor(size_t idx) const;
    bool confirmIfDestructive(const AtActionItem& action);
    std::string sendAt(const std::string& cmd, uint32_t timeoutMs=2000);
    std::string buildPromptText(const AtActionArg& a, size_t idx) const;
    static std::string joinLabel(const char* emoji, const char* text, const char* rawCmd = nullptr);
private:
    ITerminalView&     terminalView;
    IInput&            terminalInput;
    IUtilityService&   utilityService;
    UserInputManager&  userInputManager;
    ArgTransformer&    argTransformer;
    UartService&       uartService;
};
