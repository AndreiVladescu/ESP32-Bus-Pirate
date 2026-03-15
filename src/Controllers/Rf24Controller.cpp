#include "Controllers/Rf24Controller.h"
#include "Vendors/wifi_atks.h"

/*
Entry point for rf24 commands
*/
void Rf24Controller::handleCommand(const TerminalCommand& cmd) {
    const std::string& root = cmd.getRoot();

    if (root == "help") handleHelp();
    else if (root == "config")     handleConfig();
    else if (root == "send")       handleSend();
    else if (root == "receive")    handleReceive();
    else if (root == "scan")       handleScan();
    else if (root == "sweep")      handleSweep();
    else if (root == "waterfall")  handleWaterfall();
    else if (root == "jam")        handleJam();
    else if (root == "setchannel") handleSetChannel();
    else                           handleHelp();
}

/*
Ensure NRF24 is configured before use
*/
void Rf24Controller::ensureConfigured() {
    if (!configured) {
        handleConfig();
        configured = true;
        return;
    }

    uint8_t ce = state.getRf24CePin();
    uint8_t csn = state.getRf24CsnPin();
    uint8_t sck = state.getRf24SckPin();
    uint8_t miso = state.getRf24MisoPin();
    uint8_t mosi = state.getRf24MosiPin();
    rf24Service.configure(csn, ce, sck, miso, mosi, deviceView.getSharedSpiInstance());
}

/*
Receive
*/
void Rf24Controller::handleReceive() {
    int channel = userInputManager.readValidatedInt("Channel (0..125)", rf24Service.getChannel(), 0, 125);

    uint8_t addrLen = userInputManager.readValidatedInt("Address length (3..5)", 5, 3, 5);
    std::string addrStr = userInputManager.readValidatedHexString(
        "RX " + std::to_string(addrLen) + " bytes address (AA 00 ...)", addrLen, true, 2
    );

    uint8_t addr[5];
    if (!argTransformer.parseHexBytes(addrStr, addr, addrLen)) {
        terminalView.println("Invalid address. Example: AA BB CC 10 20\n");
        return;
    }

    // Params
    int pipe = userInputManager.readValidatedInt("Pipe (0..5)", 1, 0, 5);
    int crcBits = userInputManager.readValidatedInt("CRC (0=off, 8, 16)", 16, 0, 16);
    int rateSel = userInputManager.readValidatedInt("Rate (0=250kbps, 1=1Mbps, 2=2Mbps)", 1, 0, 2);
    bool dynPayload = userInputManager.readYesNo("Dynamic payloads?", true);
    bool showAscii = true;

    int fixedLen = 32;
    if (!dynPayload) {
        fixedLen = userInputManager.readValidatedInt("Payload size (1..32)", 32, 1, 32);
    }

    // Configure RX
    Rf24Service::Rf24Config cfg{};
    cfg.channel = static_cast<uint8_t>(channel);
    cfg.pipe = static_cast<uint8_t>(pipe);
    memcpy(cfg.addr, addr, addrLen); 
    cfg.addrStr = addrStr;
    cfg.addrLen = addrLen;        // 3/4/5
    cfg.crcBits = crcBits;        // 0/8/16
    cfg.dataRate = rateSel;       // 0/1/2
    cfg.dynamicPayloads = dynPayload;
    cfg.fixedPayloadSize = static_cast<uint8_t>(fixedLen);

    if (!rf24Service.initRx(cfg)) {
        terminalView.println("RF24 Receive: init failed (bad addr or not ready).\n");
        return;
    }

    terminalView.println("\n[RF24 Receive]");
    terminalView.println(" Channel : " + std::to_string(channel));
    terminalView.println(" Pipe    : " + std::to_string(pipe));
    terminalView.println(" Addr    : " + addrStr);
    terminalView.println(" CRC     : " + std::to_string(crcBits));
    terminalView.println(" Rate    : " + std::string(rateSel == 0 ? "250k" : (rateSel == 1 ? "1M" : "2M")));
    terminalView.println(" Dyn     : " + std::string(dynPayload ? "on" : "off"));
    terminalView.println("\nRF24: Receiving... Press [ENTER] to stop.");

    rf24Service.startListening();

    while (true) {
        // Cancel
        char c = terminalInput.readChar();
        if (c == '\n' || c == '\r') break;

        // Read frames
        uint8_t pipeNum = 7;
        if (rf24Service.availablePipe(&pipeNum)) {
            uint8_t buf[32] = {0};
            uint8_t len = 0;

            if (rf24Service.receive(buf, sizeof(buf), len)) {
                terminalView.println(
                    "[RX pipe=" + std::to_string(pipeNum) +
                    " len=" + std::to_string(len) + "]"
                );

                terminalView.println(argTransformer.formatHexAscii(buf, len, showAscii));
                terminalView.println("");
            } else {
                // Payload invalid 
            }
        }
    }

    rf24Service.stopListening();
    rf24Service.flushRx();
    terminalView.println("\nRF24 Receive: Stopped by user.\n");
}

/*
Send 
*/
void Rf24Controller::handleSend() {
    int channel = userInputManager.readValidatedInt("Channel (0..125)", rf24Service.getChannel(), 0, 125);

    uint8_t addrLen = userInputManager.readValidatedInt("Address length (3..5)", 5, 3, 5);
    std::string addrStr = userInputManager.readValidatedHexString(
        "TX " + std::to_string(addrLen) + " bytes address (AA 00 ...)", addrLen, true, 2
    );

    uint8_t addr[5];
    if (!argTransformer.parseHexBytes(addrStr, addr, addrLen)) {
        terminalView.println("Invalid address. Example: AA BB CC 10 20\n");
        return;
    }

    // Params
    int crcBits = userInputManager.readValidatedInt("CRC (0=off, 8, 16)", 16, 0, 16);
    int rateSel = userInputManager.readValidatedInt("Rate (0=250kbps, 1=1Mbps, 2=2Mbps)", 1, 0, 2);
    bool dynPayload = userInputManager.readYesNo("Dynamic payloads?", true);

    int fixedLen = 32;
    if (!dynPayload) {
        fixedLen = userInputManager.readValidatedInt("Payload size (1..32)", 32, 1, 32);
    }

    // Send behavior
    bool repeat = userInputManager.readYesNo("Repeat send?", false);
    int repeatDelayMs = 1000;
    if (repeat) {
        repeatDelayMs = userInputManager.readValidatedInt("Repeat delay (ms)", repeatDelayMs, 1, 60000);
    }

    // Payload prompt
    terminalView.println("\n[Payload format]");
    terminalView.println(" Text: hello world");
    terminalView.println(" Hex : hex{ AA BB CC 10 }");
    terminalView.println(" Max : 32 bytes or defined length\n");

    std::string patternRaw = userInputManager.readString("Payload", "hello world");

    // Build payload
    std::string textPattern;
    std::vector<uint8_t> hexPattern;
    std::vector<uint8_t> hexMask;
    bool isHex = false;

    if (!argTransformer.parsePattern(patternRaw, textPattern, hexPattern, hexMask, isHex)) {
        terminalView.println("RF24 Send: invalid payload. Aborted.\n");
        return;
    }

    std::vector<uint8_t> payload;

    if (isHex) {
        payload = hexPattern;
    } else {
        // decodeEscapes already done
        payload.assign(textPattern.begin(), textPattern.end());
    }

    if (payload.empty()) {
        terminalView.println("RF24 Send: empty payload. Aborted.\n");
        return;
    }

    // Enforce max
    if (payload.size() > 32) {
        terminalView.println("RF24 Send: payload too long (" + std::to_string(payload.size()) + " > 32). Truncating.\n");
        payload.resize(32);
    }

    // If fixed payload mode, pad to fixedLen
    if (!dynPayload) {
        if ((int)payload.size() < fixedLen) {
            payload.resize((size_t)fixedLen, 0x00);
        } else if ((int)payload.size() > fixedLen) {
            payload.resize((size_t)fixedLen);
        }
    }

    // Configure TX
    Rf24Service::Rf24Config cfg{};
    cfg.channel = static_cast<uint8_t>(channel);
    memcpy(cfg.addr, addr, addrLen); 
    cfg.addrStr = addrStr;
    cfg.addrLen = addrLen;       // 3/4/5
    cfg.crcBits = crcBits;       // 0/8/16
    cfg.dataRate = rateSel;      // 0/1/2
    cfg.dynamicPayloads = dynPayload;
    cfg.fixedPayloadSize = static_cast<uint8_t>(fixedLen);

    if (!rf24Service.initTx(cfg)) {
        terminalView.println("RF24 Send: init failed (bad addr or not ready).\n");
        return;
    }

    // Summary
    terminalView.println("\n[RF24 Send]");
    terminalView.println(" Channel : " + std::to_string(channel));
    terminalView.println(" Addr    : " + addrStr);
    terminalView.println(" CRC     : " + std::to_string(crcBits));
    terminalView.println(" Rate    : " + std::string(rateSel == 0 ? "250k" : (rateSel == 1 ? "1M" : "2M")));
    terminalView.println(" Dyn     : " + std::string(dynPayload ? "on" : "off"));
    if (!dynPayload) terminalView.println(" Len     : " + std::to_string(fixedLen));
    terminalView.println(" Payload : " + std::to_string((int)payload.size()) + " bytes");
    terminalView.println("");

    terminalView.println(argTransformer.formatHexAscii(payload.data(), (uint8_t)payload.size(), true));
    terminalView.println("");

    // Send loop
    if (!repeat) {
        bool ok = rf24Service.send(payload.data(), (uint8_t)payload.size());
        terminalView.println(std::string(ok ? "TX SENT ACK OK\n" : "TX SENT NO ACK\n"));
        rf24Service.flushTx();
        return;
    }

    terminalView.println("RF24: Sending... Press [ENTER] to stop.\n");

    uint32_t count = 0;
    while (true) {
        char c = terminalInput.readChar();
        if (c == '\n' || c == '\r') break;

        bool ok = rf24Service.send(payload.data(), (uint8_t)payload.size());
        count++;

        terminalView.println(
            "[TX #" + std::to_string(count) + " " + (ok ? "SENT ACK OK" : "SENT NO ACK") + " len=" + std::to_string((int)payload.size()) + "]"
        );

        delay(repeatDelayMs);
    }

    rf24Service.flushTx();
    terminalView.println("\nRF24 Send: Stopped by user.\n");
}

/*
Scan
*/
void Rf24Controller::handleScan() {
    uint32_t dwell = 128; // µs 
    uint8_t levelHold[126 + 1] = {0}; // 0..200
    uint8_t threshold = userInputManager.readValidatedUint8("High threshold (10..200)?", 20, 10, 200);
    int bestCh = -1;
    uint8_t bestVal = 0;
    const uint8_t DECAY = 6;  // decay per sweep
    
    terminalView.println("RF24: Scanning channel 0 to 125... Press [ENTER] to stop.\n");
    
    rf24Service.initRx();
    while (true) {
        // Cancel
        char c = terminalInput.readChar();
        if (c == '\n' || c == '\r') break;

        // Sweep
        for (uint8_t ch = 0; ch <= 125; ++ch) {
            rf24Service.setChannel(ch);
            rf24Service.startListening();
            delayMicroseconds(dwell);
            rf24Service.stopListening();

            // Measure
            uint8_t instant = 0;
            if (rf24Service.testRpd())          instant = 200;
            else if (rf24Service.testCarrier()) instant = 120;

            // Decrease hold level
            if (levelHold[ch] > DECAY) levelHold[ch] -= DECAY;
            else levelHold[ch] = 0;

            // Get max
            if (instant > levelHold[ch]) levelHold[ch] = instant;

            if (levelHold[ch] >= threshold) {
                // Save best
                if (levelHold[ch] > bestVal) {
                    bestVal = levelHold[ch];
                    bestCh = ch;
                }

                // Log
                terminalView.println(
                    "  Detect: Channel=" + std::to_string(ch) +
                    "  Freq=" + std::to_string(2400 + ch) + " MHz" +
                    "  Level=" + std::to_string(levelHold[ch])
                );
            }
        }
    }

    // Result
    terminalView.println("");
    if (bestCh >= 0) {
        // Log best
        terminalView.println(
            "Best channel: ch=" + std::to_string(bestCh) +
            "  f=" + std::to_string(2400 + bestCh) + " MHz" +
            "  peakLevel=" + std::to_string(bestVal)
        );

        // Ask to apply to config
        if (userInputManager.readYesNo("Save best channel to config?", true)) {
            rf24Service.setChannel((uint8_t)bestCh);
            terminalView.println("RF24: Channel set to " + std::to_string(bestCh) + ".\n");
        } else {
            terminalView.println("RF24: Channel not changed.\n");
        }
    } else {
        terminalView.println("\nRF24: No activity detected.\n");
    }
}

/*
Jam
*/
void Rf24Controller::handleJam() {
    auto confirm = userInputManager.readYesNo("RF24 Jam: This will transmit random signals. Continue?", false);
    if (!confirm) return;

    // List of group names
    std::vector<std::string> groupNames;
    groupNames.reserve(RF24_GROUP_COUNT);
    for (size_t i = 0; i < RF24_GROUP_COUNT; ++i) groupNames.emplace_back(RF24_GROUPS[i].name);
    groupNames.emplace_back(" All channels");
    groupNames.emplace_back(" One Channel");
    groupNames.emplace_back(" Exit");

    // Select group
    int choice = userInputManager.readValidatedChoiceIndex("Select band to jam:", groupNames, 0);

    // Exit selected
    if (choice == (int)groupNames.size() - 1) {
        terminalView.println("RF24 Jam: Exiting...\n");
        return;
    }

    // Prepare channel list based on choice
    uint8_t fullRangeChannels[126];
    for (uint8_t i = 0; i <= 125; ++i) fullRangeChannels[i] = i;
    uint8_t oneRange[1];
    Rf24ChannelGroup localGroup = {" Local", nullptr, 0};

    // Get group
    const Rf24ChannelGroup* group = nullptr;
    if (groupNames[choice].find("All channels") != std::string::npos) {
        // Full range
        localGroup = { " All channels", fullRangeChannels, 126 };
        group = &localGroup;;
    } else if (groupNames[choice].find("One Channel") != std::string::npos) {
        // One channel
        uint8_t ch = userInputManager.readValidatedUint8("Channel to jam (0..125)", 0, 0, 125);
        oneRange[0] = ch;
        localGroup = { " One channel", oneRange, 1 };
        group = &localGroup;
    } else {
        // WiFi, Bluetooth, etc
        group = &RF24_GROUPS[choice];
    }
    bool isWiFi  = groupNames[choice].find("WiFi") != std::string::npos;
    
    terminalView.println("\nRF24: Sweeping noise on channels... Press [ENTER] to stop.");

    rf24Service.stopListening();
    rf24Service.setDataRate(RF24_2MBPS);
    rf24Service.powerUp();
    rf24Service.setPowerMax();

    // WiFi beacon setup for wifi group
    const uint8_t wifiCh[3] = { 1, 6, 11 };
    constexpr size_t wifiChCount = sizeof(wifiCh) / sizeof(wifiCh[0]);
    uint8_t chIdx = 0;

    // Jam loop
    bool run = true;
    while (run) {
        for (size_t i = 0; i < group->count; ++i) {
            // Cancel
            int k = terminalInput.readChar();
            if (k == '\n' || k == '\r') { run = false; break; }

            // Sweep
            rf24Service.setChannel(group->channels[i]);

            // also spam with WiFi beacons
            if (isWiFi) {
                beaconCreate("", wifiCh[chIdx], true); // from wifi_atks
                chIdx = (chIdx + 1) % wifiChCount;
            }
        }
    }

    rf24Service.flushTx();
    rf24Service.powerDown();
    terminalView.println("RF24: Jam stopped by user.\n");
}

/*
Sweep
*/
void Rf24Controller::handleSweep() {
    // Params
    int dwellMs  = userInputManager.readValidatedInt("Hold time per channel (ms)", 10, 10, 1000);
    int samples  = userInputManager.readValidatedInt("Samples per channel", 80, 1, 100);
    int thrPct   = userInputManager.readValidatedInt("Activity threshold (%)", 1, 0, 100);

    terminalView.println("\nRF24 Sweep: channels 0–125"
                         " | hold=" + std::to_string(dwellMs) + " ms"
                         " | samples=" + std::to_string(samples) +
                         " | thr=" + std::to_string(thrPct) + "%... Press [ENTER] to stop.\n");

    bool run = true;
    while (run) {
        for (int ch = 0; ch <= 125 && run; ++ch) {
            // Cancel
            char c = terminalInput.readChar();
            if (c == '\n' || c == '\r') { run = false; break; }

            rf24Service.setChannel(static_cast<uint8_t>(ch));

            // Analyze activity
            int hits = 0;
            for (int s = 0; s < samples; ++s) {
                rf24Service.startListening();
                delayMicroseconds((dwellMs * 1000) / samples);
                rf24Service.stopListening();
                if (rf24Service.testRpd()) {
                    hits += 2;
                }

                if  (rf24Service.testCarrier()) {
                    hits++;
                }
            }

            int activityPct = (hits * 100) / samples;
            if (activityPct > 100) activityPct = 100;

            // Log if above threshold
            if (activityPct >= thrPct) {
                terminalView.println(
                    "  Channel " + std::to_string(ch) + " (" +
                    std::to_string(2400 + ch) + " MHz)" +
                    "  activity=" + std::to_string(activityPct) + "%"
                );
            }
        }
    }

    rf24Service.flushRx();
    terminalView.println("\nRF24 Sweep: Stopped by user.\n");
}

/*
Waterfall
*/
void Rf24Controller::handleWaterfall()
{
    // Window ms per channel
    int dwellMs = userInputManager.readValidatedInt("Hold time per channel (ms)", 10, 2, 2000);

    terminalView.println("\nRF24 Waterfall: Displaying on the ESP32 screen... Press [ENTER] to stop.\n");
    
    // sampling period 160µs so RPD has time to latch
    const uint8_t SAMPLE_PERIOD_US = 160;
    int samples = (dwellMs * 1000) / SAMPLE_PERIOD_US;
    uint8_t ch = 0;
    int bestChannel = -1;
    int bestLevel   = 0;
    std::string title = "Peak: --";

    rf24Service.initRx();

    while (true) {
        // Cancel on ENTER
        char c = terminalInput.readChar();
        if (c == '\n' || c == '\r') break;

        rf24Service.setChannel((uint8_t)ch);

        // Same logic as sweep
        int hits = 0;
        for (int s = 0; s < samples; ++s) {
            rf24Service.startListening();
            delayMicroseconds((dwellMs * 1000) / samples);
            rf24Service.stopListening();

            if (rf24Service.testRpd())   hits += 2;
            if (rf24Service.testCarrier()) hits += 1;
        }

        // normalization
        int level = (hits * 100) / samples;
        if (level < 1) level = 1; // see the waterfall progress
        if (level > 100) level = 100;

        if (level > bestLevel) {
            bestLevel = level;
            bestChannel = ch;
        }

        // Update title with best channel
        if (bestChannel >= 0 && bestLevel > 1) {
            title = "Peak: CH" + std::to_string(bestChannel) +
                    " (" + std::to_string(2400 + bestChannel) + "MHz)";
        }

        // Draw the line for this channel
        deviceView.drawWaterfall(
            title,
            0.0f,
            125.0f,
            "ch",
            ch,
            126,
            level
        );

        // Update channel, reset at 0
        if (ch == 0) {
            title = "Peak: --";
            bestChannel = -1;
            bestLevel = 0;
        }
        ch++;
        if (ch > 125) ch = 0;
    }

    rf24Service.stopListening();
    rf24Service.flushRx();
    terminalView.println("RF24 Waterfall: Stopped by user.\n");
}

/*
Set Channel
*/
void Rf24Controller::handleSetChannel() {
    uint8_t ch = userInputManager.readValidatedUint8("Channel (0..125)?", 76, 0, 125);
    rf24Service.setChannel(ch);
    terminalView.println("RF24: Channel set to " + std::to_string(ch) + ".");
}

/*
NRF24 Configuration
*/
void Rf24Controller::handleConfig() {
    uint8_t csn = userInputManager.readValidatedInt("NRF24 CSN GPIO?", state.getRf24CsnPin());
    uint8_t sck  = userInputManager.readValidatedInt("NRF24 SCK GPIO?", state.getRf24SckPin());
    uint8_t miso = userInputManager.readValidatedInt("NRF24 MISO GPIO?", state.getRf24MisoPin());
    uint8_t mosi = userInputManager.readValidatedInt("NRF24 MOSI GPIO?", state.getRf24MosiPin());
    uint8_t ce  = userInputManager.readValidatedInt("NRF24 CE GPIO?", state.getRf24CePin());
    state.setRf24CsnPin(csn);
    state.setRf24SckPin(sck);
    state.setRf24MisoPin(miso);
    state.setRf24MosiPin(mosi);
    state.setRf24CePin(ce);

    bool ok = rf24Service.configure(csn, ce, sck, miso, mosi, deviceView.getSharedSpiInstance());

    configured = true; // consider configured even if not detected to avoid re-asking
    terminalView.println(ok ? "\n ✅ NRF24 detected and configured.\n" : "\n ❌ NRF24 not detected. Check wiring.\n");
}

/*
Help
*/
void Rf24Controller::handleHelp() {
    terminalView.println("\nUnknown command. Available RF24 commands:");
    helpShell.run(state.getCurrentMode(), false);
}
