#include "FmBroadcastShell.h"

#include <sstream>
#include <iomanip>

FmBroadcastShell::FmBroadcastShell(
    ITerminalView& terminalView,
    IInput& terminalInput,
    UserInputManager& userInputManager,
    ArgTransformer& argTransformer,
    IFmService& fmService
)
    : terminalView(terminalView),
      terminalInput(terminalInput),
      userInputManager(userInputManager),
      argTransformer(argTransformer),
      fmService(fmService)
{}

void FmBroadcastShell::run()
{
    // Ensure chip is there
    if (!fmService.begin()) {
        terminalView.println("❌ FM: Si4713 not detected (I2C 0x63). Check wiring/power.\n");
        return;
    }

    while (true) {
        terminalView.println("\n=== FM Broadcast Shell ===");
        terminalView.println("Transmits connected jack audio as an FM station.\n");

        int idx = userInputManager.readValidatedChoiceIndex("Choose action", kActions, kActionCount, 0);

        if (idx < 0 || kActions[idx] == " 🚪 Exit Shell") {
            terminalView.println("Exiting FM Broadcast Shell...\n");
            break;
        }

        switch (idx) {
            case 0: cmdStatus_(); break;
            case 1: cmdStart_(); break;
            case 2: cmdStop_(); break;
            case 3: cmdSetFreq_(); break;
            case 4: cmdAutoFreq_(); break;
            case 5: cmdSetPower_(); break;
            case 6: cmdSetRdsPs_(); break;
            case 7: cmdSetRdsText_(); break;
            case 8: cmdToggleTa_(); break;

            default:
                terminalView.println("❌ Unknown choice.\n");
                break;
        }
    }
}

void FmBroadcastShell::cmdStart_()
{
    // begin in case bus changed
    if (!fmService.begin()) {
        terminalView.println("❌ FM: chip not responding.\n");
        return;
    }

    // TX power
    auto txPower = fmService.getTxPower();
    if (txPower == 0) {
        txPower = userInputManager.readValidatedUint8("Initial TX power (0..115)", 88, 0, 115);
    }

    // Apply TX power
    if (!fmService.setTxPower(txPower)) {
        terminalView.println("\n❌ TX: set power failed.");
        return;
    }

    // Tune
    uint16_t f10 = fmService.getFrequency();
    if (f10 == 0) {
        float freq = userInputManager.readValidatedFloat("Initial frequency MHz", 87.30f, 87.0f, 108.0f);
        f10 = (uint16_t)(freq * 100);
    }

    if (!fmService.tune(f10)) {
        terminalView.println("\n❌ TX: tune failed.");
        return;
    }

    if (ps_.empty() || text_.empty()) {
        auto confirm = userInputManager.readYesNo("Start Radio Data System?", true);
        if (confirm) {
            if (ps_.empty()) cmdSetRdsPs_();
            if (text_.empty()) cmdSetRdsText_();
        }
    }

    // RDS optional
    if (!ps_.empty() && !text_.empty()) {
        (void)fmService.beginRds();

        std::string ps = ps_;
        if (ps.size() > 8) ps = ps.substr(0, 8);
        fmService.setRdsStation(ps.c_str());
        fmService.setRdsText(text_.c_str());
        fmService.setTrafficAnnouncement(fmService.isTaEnabled());
    }

    std::stringstream ss;
    ss << "\n✅ TX ON @ " << std::fixed << std::setprecision(2) << (f10 / 100.0f)
       << " MHz  power=" << (int)fmService.getTxPower() << " dBuV"
       << "  RDS=" << (fmService.isRdsEnabled() ? "on" : "off")
       << "  TA=" << (fmService.isTaEnabled() ? "on" : "off");
    terminalView.println(ss.str());
}

void FmBroadcastShell::cmdStop_()
{
    fmService.stop();
    terminalView.println("\n🛑 TX stopped (power 0 + reset).");
}

void FmBroadcastShell::cmdSetFreq_()
{
    float mhz = userInputManager.readValidatedFloat("Frequency MHz", 87.30f, 87.0f, 108.0f);

    uint16_t f10 = (uint16_t)(mhz * 100);
    fmService.tune(f10);

    std::stringstream ss;
    ss << "\n🎛️  Frequency set to "
       << std::setw(3) << std::setfill(' ') << (f10 / 100)
       << "."
       << std::setw(2) << std::setfill('0') << (f10 % 100)
       << " MHz";
    terminalView.println(ss.str());
}

void FmBroadcastShell::cmdAutoFreq_()
{
    terminalView.println("\nScanning best free frequency (this may take 10 sec)...");
    uint16_t best = fmService.scanBestFrequency(8750, 10800, 10);
    if (best == 0) {
        terminalView.println("\n❌ Auto frequency: scan failed.");
        return;
    }

    // Apply
    if (!fmService.tune(best)) {
        terminalView.println("\n⚠️  Found freq but tune failed.");
        return;
    }

    std::stringstream ss;
    ss << "\n✅ Auto frequency: "
       << std::fixed << std::setprecision(2) << (best / 100.0f) << " MHz";
    terminalView.println(ss.str());
}

void FmBroadcastShell::cmdSetPower_()
{
    int p = userInputManager.readValidatedInt("TX power (0..115)", (int)fmService.getTxPower(), 0, 115);
    // int antCap = userInputManager.readValidatedInt("Antenna cap (0..15)", 0, 0, 15); // can make the Si4713 irresponsive, so skip for now
    bool ok = fmService.setTxPower((uint8_t)p);
    terminalView.println(ok ? "\n✅ Power updated." : "\n❌ Power set failed.");
}

void FmBroadcastShell::cmdSetRdsPs_()
{
    std::string ps = userInputManager.readSanitizedString("RDS Name", ps_.empty() ? "ESP32BPF" : ps_, false);
    if (ps.size() > 8) ps = ps.substr(0, 8); // RDS PS practical max length
    ps_ = ps;
    if (!ps_.empty() && !text_.empty() && !fmService.isRdsEnabled()) {
        (void)fmService.beginRds();
        fmService.setRdsStation(ps_.c_str());
        fmService.setRdsText(text_.c_str());
    }
    terminalView.println("\n🆔 RDS PS set to: " + ps_);
}

void FmBroadcastShell::cmdSetRdsText_()
{
    std::string txt = userInputManager.readString("RDS Text", text_.empty() ? "Hello from ESP32BP FM" : text_);
    if (txt.empty()) {
        terminalView.println("Empty, RDS disabled.\n");
        return;
    }
    if (txt.size() > 64) txt = txt.substr(0, 64); // RDS RadioText max practical length

    text_ = txt;
    if (!ps_.empty() && !text_.empty() && !fmService.isRdsEnabled()) {
        (void)fmService.beginRds();
        fmService.setRdsStation(ps_.c_str());
        fmService.setRdsText(text_.c_str());
    }

    terminalView.println("\n💬 RDS text updated.");
}

void FmBroadcastShell::cmdToggleTa_()
{
    if (!fmService.isTaEnabled()) {
        auto confirm = userInputManager.readYesNo("Enable Traffic Announcement (TA)?", false);
        if (confirm) {
            terminalView.println("\n🚦 TA enabled. Receiver will switch to this station.");
            fmService.setTrafficAnnouncement(true);
        }
            
    } else {
        terminalView.println("\n🚦 TA disabled. Receiver will no longer switch to this station.");
        fmService.setTrafficAnnouncement(false);
    }
}

void FmBroadcastShell::cmdStatus_()
{
    uint16_t f10 = fmService.getFrequency();

    std::stringstream ss;
    ss << "\n=== FM Broadcast status ===\r\n";
    ss << "  initialized : " << (fmService.isInitialized() ? "yes" : "no") << "\r\n";
    ss << "  running     : " << (fmService.isRunning() ? "yes" : "no") << "\r\n";
    ss << "  frequency   :" << std::setw(3) << (f10 / 100)
       << "." << std::setw(2) << std::setfill('0') << (f10 % 100) << " MHz\r\n";
    ss << "  power       : " << (int)fmService.getTxPower() << " dBuV\r\n";
    ss << "  radio data  : " << (fmService.isRdsEnabled() ? "on" : "off") << "\r\n";
    ss << "  traffic an  : " << (fmService.isTaEnabled() ? "on" : "off") << "\r\n";
    ss << "  rds name    : '" << ps_ << "'\r\n";
    ss << "  rds text    : '" << text_ << "'\r";

    terminalView.println(ss.str());
}
