#include "IbuttonShell.h"
#include <sstream>
#include <iomanip>
#include <cstring>

IbuttonShell::IbuttonShell(ITerminalView& terminalView,
                           IInput& terminalInput,
                           IUtilityService& utilityService,
                           UserInputManager& userInputManager,
                           ArgTransformer& argTransformer,
                           OneWireService& oneWireService)
    : terminalView(terminalView),
      terminalInput(terminalInput),
      utilityService(utilityService),
      userInputManager(userInputManager),
      argTransformer(argTransformer),
      oneWireService(oneWireService) {}

void IbuttonShell::run() {
    const std::vector<std::string> actions = {
        " 🔍 Read ID",
        " ✏️  Write ID",
        " 📋 Copy ID",
        " 🚪 Exit Shell"
    };

    while (true) {
        terminalView.println("\n=== iButton RW1990 Shell ===");
        int index = userInputManager.readValidatedChoiceIndex("Select an action", actions, 0);

        if (index == -1 || actions[index] == " 🚪 Exit Shell") {
            terminalView.println("Exiting iButton Shell...\n");
            break;
        }

        switch (index) {
            case 0: cmdReadId(); break;
            case 1: cmdWriteId(); break;
            case 2: cmdCopyId(); break;
            default:
                terminalView.println("❌ Unknown choice. Using default action.\n");
                break;
        }
    }
}

void IbuttonShell::cmdReadId() {
    terminalView.println("iButton Read: Press [ENTER] to stop.\n");

    while (true) {
        auto key = terminalInput.readChar();
        if (key == '\r' || key == '\n') {
            terminalView.println("\niButton Read: Stopped by user.");
            break;
        }
        utilityService.sleepMs(100);

        uint8_t buffer[8];
        if (!oneWireService.reset()) continue;

        terminalView.println("iButton Read: In progress...");
        oneWireService.write(0x33);  // Read ROM
        oneWireService.readBytes(buffer, 8);

        std::ostringstream oss;
        oss << std::uppercase << std::hex << std::setfill('0');
        for (int i = 0; i < 8; ++i) {
            oss << std::setw(2) << static_cast<int>(buffer[i]);
            if (i < 7) oss << " ";
        }

        terminalView.println("ROM ID: " + oss.str());

        uint8_t crc = oneWireService.crc8(buffer, 7);
        if (crc != buffer[7]) {
            terminalView.println("❌ CRC error on ROM ID.");
        }

        break;
    }
}

void IbuttonShell::cmdWriteId() {
    terminalView.println("iButton ID Write: Enter 8 bytes ID (ex: 01 AA 03 BB 05 FF 07 08)");

    std::string hexStr = userInputManager.readValidatedHexString("Enter ROM ID (8 bytes)", 8);
    std::vector<uint8_t> idBytes = argTransformer.parseHexList(hexStr);

    if (idBytes.size() != 8) {
        terminalView.println("❌ Invalid ID length. Must be exactly 8 bytes.");
        return;
    }

    const int maxRetries = 8;
    int attempt = 0;
    bool success = false;

    terminalView.println("iButton ID Write: Waiting for device... Press [ENTER] to stop");

    while (!oneWireService.reset()) {
        utilityService.sleepMs(1);
        auto key = terminalInput.readChar();
        if (key == '\r' || key == '\n') {
            terminalView.println("\niButton Write: Stopped by user.");
            return;
        }
    }

    while (attempt < maxRetries && !success) {
        attempt++;
        terminalView.println("Attempt " + std::to_string(attempt) + "...");

        oneWireService.writeRw1990(state.getOneWirePin(), idBytes.data(), idBytes.size());
        utilityService.sleepMs(50);

        uint8_t buffer[8];
        if (!oneWireService.reset()) continue;
        oneWireService.write(0x33);
        oneWireService.readBytes(buffer, 8);

        if (memcmp(buffer, idBytes.data(), 7) != 0) {
            terminalView.println("❌ Mismatch in ROM ID bytes.");
            continue;
        }

        success = true;
    }

    if (success) terminalView.println("✅ ID write successful.");
    else         terminalView.println("❌ ID write failed.");
}

void IbuttonShell::cmdCopyId() {
    terminalView.println("iButton Copy: Insert source tag... Press [ENTER] to stop\n");

    // Wait for source tag
    while (!oneWireService.reset()) {
        auto key = terminalInput.readChar();
        if (key == '\r' || key == '\n') {
            terminalView.println("\niButton Copy: Stopped by user.");
            return;
        }
        utilityService.sleepMs(100);
    }

    // Read source ID
    uint8_t id[8];
    oneWireService.write(0x33);  // Read ROM
    oneWireService.readBytes(id, 8);
    std::vector<uint8_t> idVec(id, id + 8); 

    // Print ID
    std::ostringstream oss;
    oss << std::uppercase << std::hex << std::setfill('0');
    for (int i = 0; i < 8; ++i) {
        oss << std::setw(2) << static_cast<int>(idVec[i]);
        if (i < 7) oss << " ";
    }
    terminalView.println("ROM ID: " + oss.str());

    // Prompt to insert target
    terminalView.println("Remove source tag and insert target clone... Press [ENTER] when ready.");
    while (true) {
        auto c = terminalInput.readChar();
        if (c == '\r' || c == '\n') {
            terminalView.println("Starting ID write...");
            break;
        }
    }

    // Wait for target tag
    const int maxRetries = 8;
    int attempt = 0;
    bool success = false;

    while (!oneWireService.reset()) {
        auto key = terminalInput.readChar();
        if (key == '\r' || key == '\n') {
            terminalView.println("\niButton Copy: Stopped by user.");
            return;
        }
        utilityService.sleepMs(1);
    }

    // Attempt writing and verification
    while (attempt < maxRetries && !success) {
        attempt++;
        terminalView.println("Attempt " + std::to_string(attempt) + "...");

        // Write ID to target
        oneWireService.writeRw1990(state.getOneWirePin(), idVec.data(), idVec.size());
        utilityService.sleepMs(50);

        // Verify
        uint8_t buffer[8];
        if (!oneWireService.reset()) continue;
        oneWireService.write(0x33);
        oneWireService.readBytes(buffer, 8);

        if (memcmp(buffer, idVec.data(), 7) != 0) {
            terminalView.println("Mismatch in ROM ID bytes.");
            continue;
        }

        success = true;
        break;
    }

    if (success) {
        terminalView.println("✅ Copy complete.");
    } else {
        terminalView.println("❌ Failed to copy ID.");
    }
}
