#pragma once

#include <vector>
#include <string>
#include <numeric>
#include <sstream>
#include <cctype>
#include "Interfaces/ITerminalView.h"
#include "Interfaces/IInput.h"
#include "Interfaces/IDeviceView.h"
#include "Interfaces/IUtilityService.h"
#include "Models/TerminalCommand.h"
#include "Models/ByteCode.h"
#include "Transformers/ArgTransformer.h"
#include "Transformers/SubGhzTransformer.h"
#include "Managers/UserInputManager.h"
#include "Analyzers/SubGhzAnalyzer.h"
#include "States/GlobalState.h"
#include "Interfaces/ISubGhzService.h"
#include "Interfaces/IPinService.h"
#include "Interfaces/ILittleFsService.h"
#include "Interfaces/II2sService.h"
#include "Data/SubGhzProtocols.h"
#include "Shells/HelpShell.h"

class SubGhzController {
public:
    SubGhzController(ITerminalView& terminalView,
                     IInput& terminalInput,
                     IDeviceView& deviceView,
                     IUtilityService& utilityService,
                     ISubGhzService& subGhzService,
                     IPinService& pinService,
                     II2sService& i2sService,
                     ILittleFsService& littleFsService,
                     ArgTransformer& argTransformer,
                     SubGhzTransformer& subGhzTransformer,
                     UserInputManager& userInputManager,
                     SubGhzAnalyzer& subGhzAnalyzer,
                     HelpShell& helpShell)
    : terminalView(terminalView),
      terminalInput(terminalInput),
      deviceView(deviceView),
      utilityService(utilityService),
      subGhzService(subGhzService),
      pinService(pinService),
      i2sService(i2sService),
      littleFsService(littleFsService),
      argTransformer(argTransformer),
      subGhzTransformer(subGhzTransformer),
      userInputManager(userInputManager),
      subGhzAnalyzer(subGhzAnalyzer),
      helpShell(helpShell) {}

    // Entry point for subghz commands
    void handleCommand(const TerminalCommand& cmd);

    // Ensure subghz mode is properly configured before use
    void ensureConfigured();

private:
    // Scan for frequencies
    void handleScan(const TerminalCommand& cmd);

    // Set current frequency
    void handleSetFrequency();

    // Replay raw frames with a dedicated window
    void handleReplayRaw(const TerminalCommand& cmd);

    // Replay decoded frames 
    void handleReplayDecoded(const TerminalCommand& cmd);

    // Jam signals
    void handleJam(const TerminalCommand& cmd);

    // Band Jam
    void handleBandJam();
    
    // Decoded signals reception
    void handleDecode(const TerminalCommand& cmd);
    
    // Raw signals reception 
    void handleRaw(const TerminalCommand& cmd);

    // Decode signals or receive raw symbols
    void handleReceive(const TerminalCommand& cmd);

    // Show signal trace
    void handleTrace();

    // Show frequency peak waterfall
    void handleWaterfall();

    // Sweep and analyze signals
    void handleSweep();

    // Bruteforce attack
    void handleBruteforce();

    // Load .sub files
    void handleLoad();

    // Record raw/decoded signals to .sub files
    void handleRecord();

    // Convert RSSI to audio
    void handleEar();
    
    // Send payload
    void handleSend(const TerminalCommand& cmd);

    // Configure CC1101
    bool handleConfig();

    // Available commands
    void handleHelp();

private:
    ITerminalView& terminalView;
    IInput& terminalInput;
    IDeviceView& deviceView;
    IUtilityService& utilityService;
    ISubGhzService& subGhzService;
    IPinService& pinService;
    II2sService& i2sService;
    ILittleFsService& littleFsService;
    ArgTransformer& argTransformer;
    SubGhzTransformer& subGhzTransformer;
    UserInputManager& userInputManager;
    SubGhzAnalyzer& subGhzAnalyzer;
    HelpShell& helpShell;
    GlobalState& state = GlobalState::getInstance();

    bool configured = false;
};
