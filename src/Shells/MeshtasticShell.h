#pragma once

#include <string>
#include <vector>

#include "Interfaces/IInput.h"
#include "Interfaces/ITerminalView.h"
#include "Interfaces/IUtilityService.h"
#include "Managers/UserInputManager.h"
#include "Models/MeshtasticPacket.h"
#include "Models/LoRaRadioProfile.h"
#include "Services/LoRaService.h"
#include "Services/MeshtasticService.h"
#include "Transformers/ArgTransformer.h"

class MeshtasticShell {
public:
    MeshtasticShell(ITerminalView& terminalView,
                    IInput& terminalInput,
                    IUtilityService& utilityService,
                    UserInputManager& userInputManager,
                    ArgTransformer& argTransformer,
                    LoRaService& loRaService,
                    MeshtasticService& meshtasticService);

    void run();

private:
    inline static constexpr const char* kActions[] = {
        " Status",
        " Select preset",
        " Set frequency",
        " Set channel key",
        " Clear channel key",
        " Send text",
        " Receive packets",
        " Inspect hex frame",
        " Exit Shell"
    };

    static constexpr size_t kActionCount =
        sizeof(kActions) / sizeof(kActions[0]);

    bool applyProfile();
    void cmdStatus();
    void cmdPreset();
    void cmdFrequency();
    void cmdSetKey();
    void cmdClearKey();
    void cmdSend();
    void cmdReceive();
    void cmdInspect();
    void printPacket(const MeshtasticPacket& packet,
                     const std::vector<uint8_t>& frame,
                     uint32_t index = 0);
    void printWrapped(const std::string& label,
                      const std::string& value,
                      size_t width = 28);
    std::string nodeId(uint32_t value);
    static std::string channelNameForPreset(const std::string& preset);

    ITerminalView& terminalView;
    IInput& terminalInput;
    IUtilityService& utilityService;
    UserInputManager& userInputManager;
    ArgTransformer& argTransformer;
    LoRaService& loRaService;
    MeshtasticService& meshtasticService;

    std::string presetName_ = "LONG_FAST";
    std::string channelName_ = "LongFast";
    float frequency_ = 868.0f;
    uint32_t nodeNumber_ = 0;
    uint32_t nextPacketId_ = 0;
    LoRaRadioProfile profile_{};
};
