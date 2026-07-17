#pragma once

#include "Interfaces/ITerminalView.h"
#include "Interfaces/IInput.h"
#include "Interfaces/IDeviceView.h"
#include "Interfaces/IUtilityService.h"
#include "Managers/UserInputManager.h"
#include "Models/TerminalCommand.h"
#include "Interfaces/ILoRaService.h"
#include "Interfaces/ILittleFsService.h"
#include "Interfaces/II2sService.h"
#include "Interfaces/IShell.h"
#include "Shells/HelpShell.h"
#include "States/GlobalState.h"
#include "Transformers/ArgTransformer.h"
#include "Transformers/LoRaTransformer.h"
#include "Transformers/TerminalCommandTransformer.h"

class LoRaController {
public:
    LoRaController(ITerminalView& tv, IInput& input, IDeviceView& device,
                   IUtilityService& utilityService, ILoRaService& service, ILittleFsService& littleFs,
                   II2sService& i2s,
                   ArgTransformer& transformer, LoRaTransformer& loRaTransformer,
                   TerminalCommandTransformer& commandTransformer,
                   UserInputManager& uim,
                   HelpShell& help,
                   IShell& meshtasticShell);

    void handleCommand(const TerminalCommand& cmd);
    void ensureConfigured();
    void ensureReleased();

private:
    void handleConfig();
    void handleSend(const TerminalCommand& cmd);
    void handleSpam(const TerminalCommand& cmd);
    void handleJam(const TerminalCommand& cmd);
    void handleReceive();
    void handleRecord();
    void handleLoad();
    void handleRssi(const TerminalCommand& cmd);
    void handleEar();
    void handleScan();
    void handleWaterfall();
    void handleCad(const TerminalCommand& cmd);
    void handleAirtime(const TerminalCommand& cmd);
    void handleSetFrequency(const TerminalCommand& cmd);
    void handleStatus();
    
    bool mountLittleFs();
    void configurePins();
    void configureRadioProfile();
    void resetDefaultRadioProfile();

    void printCurrentProfile(const char* title = nullptr);
    int readOptionalInterval(const TerminalCommand& cmd, const std::string& label,
                             int defaultValue, int minimum, int maximum);



    ITerminalView& terminalView;
    IInput& terminalInput;
    IDeviceView& deviceView;
    IUtilityService& utilityService;
    ILoRaService& loRaService;
    ILittleFsService& littleFsService;
    II2sService& i2sService;
    ArgTransformer& argTransformer;
    LoRaTransformer& loRaTransformer;
    TerminalCommandTransformer& terminalCommandTransformer;
    UserInputManager& userInputManager;
    HelpShell& helpShell;
    IShell& meshtasticShell;
    GlobalState& state = GlobalState::getInstance();
    bool configured = false;
};
