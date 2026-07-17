#pragma once
#include <string>
#include <vector>

#include "Interfaces/ITerminalView.h"
#include "Interfaces/IInput.h"
#include "Interfaces/IUtilityService.h"
#include "Interfaces/IShell.h"
#include "Managers/UserInputManager.h"
#include "Interfaces/ILittleFsService.h"
#include "Transformers/ProfileTransformer.h"

class ProfileShell : public IShell {
public:
    ProfileShell(ITerminalView& tv,
                 IInput& in,
                 IUtilityService& utilityService,
                 UserInputManager& uim,
                 ILittleFsService& lfs,
                 ProfileTransformer& profileTransformer);

    void run() override;

private:
    ITerminalView& terminalView;
    IInput& terminalInput;
    IUtilityService& utilityService;
    UserInputManager& userInputManager;
    ILittleFsService& littleFsService;
    ProfileTransformer& profileTransformer;

    inline static constexpr const char* actions[] = {
        " 📥 Load profile",
        " 💾 Save profile",
        " 🚪 Exit"
    };
    inline static constexpr size_t actionsCount = sizeof(actions) / sizeof(actions[0]);

    inline static constexpr const char* PROFILE_EXT = ".profile";

    void cmdLoad();
    void cmdSave();

    // helpers
    bool ensureMounted();
    std::vector<std::string> listProfiles();
    std::string buildProfilePath(const std::string& baseName);
};
