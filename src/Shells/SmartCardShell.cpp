
#include "SmartCardShell.h"

SmartCardShell::SmartCardShell(
    TwoWireService& twoWireService,
    ITerminalView& terminalView,
    IInput& terminalInput,
    IUtilityService& utilityService,
    ArgTransformer& argTransformer,
    UserInputManager& userInputManager
)
    : twoWireService(twoWireService),
      terminalView(terminalView),
      terminalInput(terminalInput),
      utilityService(utilityService),
      argTransformer(argTransformer),
      userInputManager(userInputManager)
{}

void SmartCardShell::run() {
    const std::vector<std::string> actions = {
        " 🔍 Probe",
        " 🛡️  Security check",
        " 🔓 Unlock card",
        " 📝 PSC Set",
        " 📋 PSC Get",
        " ✏️  Write",
        " 🗃️  Dump",
        " 🚫 Protect",
        " 🚪 Exit Shell"
    };

    while (true) {
        terminalView.println("\n=== SLE44XX SmartCard Shell ===");
        int index = userInputManager.readValidatedChoiceIndex("Select a SmartCard action", actions, 0);

        if (index == -1 || actions[index] == " 🚪 Exit Shell") {
            terminalView.println("Exiting SmartCard Shell...\n");
            break;
        }

        switch (index) {
            case 0: cmdProbe();     break;
            case 1: cmdSecurity();  break;
            case 2: cmdUnlock();    break;
            case 3: cmdPsc("set");  break;
            case 4: cmdPsc("get");  break;
            case 5: cmdWrite();     break;
            case 6: cmdDump();      break;
            case 7: cmdProtect();   break;
            default:
                terminalView.println("Unknown choice.\n");
                break;
        }
    }
}

/*
Smartcard Security
*/
void SmartCardShell::cmdSecurity() {
    twoWireService.resetSmartCard();

    terminalView.println("2WIRE Security: Perfoming...\n");
    
    // Security memory
    terminalView.println("   [Security Memory] Command: 0x31 0x00 0x00");
    twoWireService.sendCommand(0x31, 0x00, 0x00);
    auto sec = twoWireService.readResponse(4);

    // Validate
    bool allZero = std::all_of(sec.begin(), sec.end(), [](uint8_t b) { return b == 0x00; });
    bool allFF   = std::all_of(sec.begin(), sec.end(), [](uint8_t b) { return b == 0xFF; });
    if (sec.empty() || allZero || allFF) {
        terminalView.println("2WIRE Security: ❌ No smartcard detected (response invalid)");
        return;
    }
    
    // Display
    std::stringstream secOut;
    secOut << "   Security Bytes: ";
    for (auto b : sec) secOut << "0x" << std::hex << std::setw(2) << std::setfill('0') << (int)b << " ";
    terminalView.println(secOut.str());

    if (!sec.empty()) {
        uint8_t attempts = twoWireService.parseSmartCardRemainingAttempts(sec[0]);
        terminalView.println("   Remaining Unlock Attempts: " + std::to_string(attempts));
    }

    terminalView.println("\n2WIRE Security: ✅ Completed.");
}

/*
Smartcard Probe (ATR)
*/
void SmartCardShell::cmdProbe() {
    terminalView.println("\n2WIRE ATR: Performing...\n");

    // ATR
    auto atr = twoWireService.performSmartCardAtr();
    std::stringstream ss;
    ss << "ATR: ";

    // Validate
    if (atr.empty() || atr[0] == 0x00 || atr[0] == 0xFF) {
        terminalView.println("2WIRE ATR: ❌ No response received from smartcard");
        return;
    }

    // Decode and display
    auto decodedAtr = twoWireService.parseSmartCardAtr(atr);
    for (uint8_t b : atr) {
        ss << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b) << " ";
    }
    terminalView.println(decodedAtr);
    
    twoWireService.resetSmartCard();
    terminalView.println("2WIRE ATR: ✅ Completed.");
}

/*
Smartcard Dump
*/
void SmartCardShell::cmdDump() {
    twoWireService.resetSmartCard();
    utilityService.sleepMs(10);
    terminalView.println("\n2WIRE Dump: Reading full memory (MAIN + SEC + PROTECT)...");

    // Dump 256 bytes + sec mem + protection mem
    auto dump = twoWireService.dumpSmartCardFullMemory();
    if (dump.size() != 264) {
        terminalView.println("\n2WIRE Dump: ❌ Failed, unexpected size.");
        return;
    }

    // Validate data
    bool allZero = std::all_of(dump.begin(), dump.end(), [](uint8_t b) { return b == 0x00; });
    bool allFF   = std::all_of(dump.begin(), dump.end(), [](uint8_t b) { return b == 0xFF; });
    if (allZero || allFF) {
        terminalView.println("\n2WIRE Dump: ❌ Smartcard is blank or no smartcard detected");
        return;
    }

    // Main memory (0–255)
    terminalView.println("\n[Main Memory]");
    for (int i = 0; i < 256; i += 16) {
        std::stringstream line;
        line << std::hex << std::uppercase << std::setfill('0');
        line << std::setw(2) << i << ": ";
        for (int j = 0; j < 16; ++j) {
            line << std::setw(2) << static_cast<int>(dump[i + j]) << " ";
        }
        terminalView.println(line.str());
    }

    // Security memory (256–259)
    terminalView.println("\n[Security Memory]");
    std::stringstream sec;
    sec << "SEC: ";
    for (int i = 256; i < 260; ++i) {
        sec << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(dump[i]) << " ";
    }
    uint8_t attempts = twoWireService.parseSmartCardRemainingAttempts(dump[256]);
    sec << "→ Attempts Left: " << std::dec << (int)attempts;
    terminalView.println(sec.str());

    // Protection memory (260–263)
    terminalView.println("\n[Protection Memory]");
    std::stringstream prt;
    prt << "PRT: ";
    for (int i = 260; i < 264; ++i) {
        prt << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(dump[i]) << " ";
    }
    terminalView.println(prt.str());

    twoWireService.resetSmartCard();
    terminalView.println("\n2WIRE Dump: ✅ Completed.");
}

/*
Smartcard Protect
*/
void SmartCardShell::cmdProtect() {
    twoWireService.resetSmartCard();
    terminalView.println("⚠️ The smartcard will PERMANENTLY disable writes to main memory.");
    bool confirm = userInputManager.readYesNo("Are you sure you want to lock it PERMANENTLY?", false);
    if (!confirm) {
        terminalView.println("\n❌ Lock cancelled.");
        return;
    }

    bool ok = twoWireService.protectSmartCard();
    if (ok) terminalView.println("\n✅ Smartcard successfully locked (writes disabled).");
    else    terminalView.println("\n❌ Failed to lock smartcard.");
}

/*
Smartcard Unlock
*/
void SmartCardShell::cmdUnlock() {
    twoWireService.resetSmartCard();
    terminalView.println("2WIRE Unlock: Attempting unlock procedure...\n");

    // Info block
    terminalView.println(" [ℹ️  INFORMATION] ");
    terminalView.println(" PSC (Programmable Security Code) is 3 bytes.");
    terminalView.println(" Default value is usually: FF FF FF.");
    terminalView.println(" Enter values in hexadecimal format.\n");

    // Prompt for PSC 
    auto pscStr = userInputManager.readValidatedHexString("Enter PSC (ex: FF FF FF)", 3);
    auto psc = argTransformer.parseHexList(pscStr);

    // Unlock
    bool success = twoWireService.unlockSmartCard(psc.data());

    if (success) {
        terminalView.println("\n✅ Unlock successful: Access to main memory granted.");
    } else {
        terminalView.println("\n❌ Unlock failed: PSC incorrect or no attempts remaining.");
    }

    // Number of attempts left
    auto secAfter = twoWireService.readSmartCardSecurityMemory();
    if (!secAfter.empty()) {
        uint8_t attempts = twoWireService.parseSmartCardRemainingAttempts(secAfter[0]);
        terminalView.println("→ Remaining Attempts: " + std::to_string(attempts));
    }
}

/*
Smartcard PSC
*/
void SmartCardShell::cmdPsc(const std::string& subcommand) {
    twoWireService.resetSmartCard();
    std::string arg = subcommand;

    if (arg.empty()) {
        arg = "get"; // Default to "get" if no argument provided
    }

    // GET PSC
    if (arg == "get") {
        uint8_t psc[3];
        bool ok = twoWireService.getSmartCardPSC(psc);
        if (ok) {
            terminalView.println("\nℹ️  Note: The PSC can only be read if the smartcard is unlocked.");
            std::stringstream ss;
            ss << "🔐 Current PSC: ";
            for (int i = 0; i < 3; ++i)
                ss << std::hex << std::setw(2) << std::setfill('0') << (int)psc[i] << " ";
            terminalView.println(ss.str());

        } else {
            terminalView.println("\n❌ Failed to read PSC.");
        }

    // SET PSC
    } else if (arg == "set") {
        // Prompt for PSC
        auto pscStr = userInputManager.readValidatedHexString("Enter PSC (ex: FF FF FF)", 3);
        auto psc = argTransformer.parseHexList(pscStr);

        bool ok = twoWireService.updateSmartCardPSC(psc.data());
        if (ok) {
            terminalView.println("\n✅ PSC updated successfully.");
        } else {
            terminalView.println("\nℹ️  Note: The PSC can only be set if the smartcard is unlocked.");
            terminalView.println("❌ Failed to update PSC.");
        }
    }
}

// Smartcard Write
void SmartCardShell::cmdWrite() {
    twoWireService.resetSmartCard();

    int offset = userInputManager.readValidatedUint8("Enter offset (0–255 or 0x..)", 0);
    if (offset < 0 || offset >= 256) {
        terminalView.println("\n❌ Invalid offset (must be between 0 and 255).");
        return;
    }

    int data = userInputManager.readValidatedUint8("Enter data byte (0–255 or 0x..)", 0);
    if (data < 0 || data > 0xFF) {
        terminalView.println("\n❌ Invalid data byte.");
        return;
    }

    bool ok = twoWireService.writeSmartCardMainMemory(static_cast<uint8_t>(offset), static_cast<uint8_t>(data));
    if (ok) terminalView.println("\n✅ Write successful.");
    else  {
        terminalView.println("\nℹ️  Note: Unlock the smartcard if you are not able to write.");
        terminalView.println("❌ Write failed.");
    }
}
