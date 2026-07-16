#include "Controllers/LoRaController.h"

#include <algorithm>
#include <cmath>
#include <limits>

/*
Initialize the LoRa controller dependencies
*/
LoRaController::LoRaController(ITerminalView& tv, IInput& input, IDeviceView& device,
                               IUtilityService& utilityService, LoRaService& service, LittleFsService& littleFs,
                               I2sService& i2s,
                               ArgTransformer& transformer, LoRaTransformer& transformerLoRa,
                               TerminalCommandTransformer& commandTransformer,
                               UserInputManager& uim,
                               HelpShell& help,
                               MeshtasticShell& meshShell)
    : terminalView(tv), terminalInput(input), deviceView(device), utilityService(utilityService), loRaService(service),
      littleFsService(littleFs), i2sService(i2s), argTransformer(transformer),
      loRaTransformer(transformerLoRa),
      terminalCommandTransformer(commandTransformer),
      userInputManager(uim), helpShell(help),
      meshtasticShell(meshShell) {}

/*
Entry point for LoRa commands
*/
void LoRaController::handleCommand(const TerminalCommand& cmd) {
    const std::string root = cmd.getRoot();

    if (root == "config") handleConfig();
    else if (root == "send" || root == "tx") handleSend(cmd);
    else if (root == "spam") handleSpam(cmd);
    else if (root == "jam") handleJam(cmd);
    else if (root == "receive" || root == "sniff" || root == "read") handleReceive();
    else if (root == "record") handleRecord();
    else if (root == "load") handleLoad();
    else if (root == "rssi") handleRssi(cmd);
    else if (root == "ear") handleEar();
    else if (root == "scan") handleScan();
    else if (root == "waterfall") handleWaterfall();
    else if (root == "cad" || root == "activity") handleCad(cmd);
    else if (root == "airtime" || root == "toa") handleAirtime(cmd);
    else if (root == "setfreq" || root == "setfrequency") handleSetFrequency(cmd);
    else if (root == "status") handleStatus();
    else if (root == "meshtastic" || root == "mesh") {
        ensureConfigured();
        if (configured) meshtasticShell.run();
    }
    else helpShell.run(ModeEnum::LORA, false);
}

/*
Ensure the LoRa radio is configured before use
*/
void LoRaController::ensureConfigured() {
    if (configured && loRaService.isInitialized()) return;

    terminalView.println("\n[LoRa pin configuration]");
    configurePins();

    resetDefaultRadioProfile();
    terminalView.println("\n[LoRa default profile]");
    printCurrentProfile();

    if (userInputManager.readYesNo("Configure radio settings?", false)) {
        configureRadioProfile();
    } else {
        terminalView.println("LoRa: using the default radio profile.");
    }

    configured = loRaService.configure(deviceView.getSharedSpiInstance(), state.getLoRaSckPin(),
        state.getLoRaMisoPin(), state.getLoRaMosiPin(), state.getLoRaCsPin(), state.getLoRaRstPin(),
        state.getLoRaBusyPin(), state.getLoRaDio1Pin(), state.getLoRaProfile());

    if (!configured) {
        terminalView.println("❌ LoRa SX1262 setup failed.");
        const int16_t error = loRaService.getLastError();
        if (error == -90) {
            terminalView.println("BUSY stayed high.");
        } else if (error == -91) {
            terminalView.println("No valid radio response.");
        } else {
            terminalView.println("Error: " + std::to_string(error));
        }
        terminalView.println("Check wiring and TCXO.");
        terminalView.println("Run: config\n");
        return;
    }

    terminalView.println("✅ LoRa SX1262 ready.\n");
}

/*
Release the LoRa radio
*/
void LoRaController::ensureReleased() {
    loRaService.deinitRfModule();
    configured = false;
}

/*
Configure the LoRa pins and radio profile
*/
void LoRaController::handleConfig() {
    terminalView.println("\n[LoRa pin configuration]");
    configurePins();

    resetDefaultRadioProfile();
    terminalView.println("\n[LoRa default profile]");
    printCurrentProfile();

    if (userInputManager.readYesNo("Configure radio settings?", false)) {
        configureRadioProfile();
    } else {
        terminalView.println("LoRa: using the default radio profile.");
    }

    ensureReleased();
    configured = loRaService.configure(deviceView.getSharedSpiInstance(), state.getLoRaSckPin(),
        state.getLoRaMisoPin(), state.getLoRaMosiPin(), state.getLoRaCsPin(), state.getLoRaRstPin(),
        state.getLoRaBusyPin(), state.getLoRaDio1Pin(), state.getLoRaProfile());

    if (!configured) {
        terminalView.println("❌ LoRa SX1262 setup failed.");
        const int16_t error = loRaService.getLastError();
        if (error == -90) {
            terminalView.println("BUSY stayed high.");
        } else if (error == -91) {
            terminalView.println("No valid radio response.");
        } else {
            terminalView.println("Error: " + std::to_string(error));
        }
        terminalView.println("Check wiring and TCXO.");
        terminalView.println("Run: config\n");
        return;
    }

    terminalView.println("✅ LoRa SX1262 configured.\n");
    printCurrentProfile("[Radio profile]");
    terminalView.println("");
}

/*
Send a LoRa payload
*/
void LoRaController::handleSend(const TerminalCommand& cmd) {
    ensureConfigured();
    if (!configured) return;

    std::string raw = terminalCommandTransformer.tail(cmd);
    if (raw.empty()) raw = userInputManager.readString("Payload", "Hello LoRa");

    std::vector<uint8_t> payload;
    if (!loRaTransformer.transform(raw, payload)) {
        terminalView.println("LoRa TX: invalid payload.");
        terminalView.println("Use text or hex{ AA BB CC }.\n");
        return;
    }
    if (payload.size() > 255) {
        terminalView.println("LoRa TX: payload too long.");
        terminalView.println("Maximum: 255 bytes.\n");
        return;
    }

    terminalView.println("\n[LoRa TX]");
    terminalView.println(" Length  : " + std::to_string(payload.size()) + " bytes");
    terminalView.println(" Airtime : " + std::to_string(loRaService.getTimeOnAir(payload.size())) + " ms");
    terminalView.println(argTransformer.formatHexAscii(payload.data(), payload.size(), true, 8));

    const uint32_t started = utilityService.nowMs();
    const bool ok = loRaService.send(payload.data(), payload.size());
    const uint32_t elapsed = utilityService.nowMs() - started;

    if (ok) {
        terminalView.println("LoRa TX: sent.");
        terminalView.println("Time: " +
            std::to_string(elapsed) + " ms\n");
    } else {
        terminalView.println("LoRa TX: failed.");
        terminalView.println("Time: " +
            std::to_string(elapsed) + " ms\n");
    }
}

/*
Repeatedly send a LoRa payload
*/
void LoRaController::handleSpam(const TerminalCommand& cmd) {
    ensureConfigured();
    if (!configured) return;

    std::string full = terminalCommandTransformer.tail(cmd);
    std::string payloadRaw;
    std::string intervalRaw;
    uint32_t intervalMs = 500;

    // The last numeric token is the optional interval. A single numeric token
    // remains a text payload, so `spam 123` sends "123".
    if (!full.empty()) {
        const size_t separator = full.find_last_of(' ');
        if (separator != std::string::npos && separator + 1 < full.size()) {
            const std::string maybePayload = full.substr(0, separator);
            const std::string maybeInterval = full.substr(separator + 1);
            if (argTransformer.isValidNumber(maybeInterval)) {
                payloadRaw = maybePayload;
                intervalRaw = maybeInterval;
            } else {
                payloadRaw = full;
            }
        } else {
            payloadRaw = full;
        }
    }

    if (payloadRaw.empty()) {
        terminalView.println("\n[Payload format]");
        terminalView.println("Text: hello world");
        terminalView.println("Hex: hex{ AA BB ?? }");
        payloadRaw = userInputManager.readString(
            "Payload", "Hello LoRa");
        if (payloadRaw.empty()) {
            terminalView.println("LoRa spam: empty payload.\n");
            return;
        }

        intervalMs = userInputManager.readValidatedUint32(
            "Interval between TX (ms)", intervalMs);
    } else if (!intervalRaw.empty()) {
        const uint32_t parsed = argTransformer.parseHexOrDec32(intervalRaw);
        if (parsed == 0 || parsed > 600000) {
            terminalView.println("LoRa spam: invalid interval.");
            terminalView.println("Range: 1..600000 ms.\n");
            return;
        }
        intervalMs = parsed;
    }

    if (intervalMs == 0 || intervalMs > 600000) {
        terminalView.println("LoRa spam: invalid interval.");
        terminalView.println("Range: 1..600000 ms.\n");
        return;
    }

    std::vector<uint8_t> preview;
    if (!loRaTransformer.transform(payloadRaw, preview)) {
        terminalView.println("LoRa spam: invalid payload.");
        terminalView.println("Use text or hex{ AA BB }.");
        terminalView.println("");
        return;
    }
    if (preview.size() > 255) {
        terminalView.println("LoRa spam: payload too long.");
        terminalView.println("Maximum: 255 bytes.\n");
        return;
    }

    terminalView.println("\n[LoRa spam]");
    terminalView.println("Length: " +
        std::to_string(preview.size()) + " bytes");
    terminalView.println("Airtime: " +
        std::to_string(loRaService.getTimeOnAir(preview.size())) +
        " ms");
    terminalView.println("Interval: " +
        std::to_string(intervalMs) + " ms");
    terminalView.println("Stop: press [ENTER]\n");

    uint32_t sent = 0;
    uint32_t failed = 0;
    uint32_t lastSendAt = 0;
    uint32_t lastReportAt = utilityService.nowMs();
    const uint32_t startedAt = utilityService.nowMs();
    bool firstSend = true;

    while (true) {
        const char c = terminalInput.readChar();
        if (c == '\r' || c == '\n') break;

        const uint32_t now = utilityService.nowMs();
        if (!firstSend && now - lastSendAt < intervalMs) {
            if (now - lastReportAt >= 5000) {
                terminalView.println("[Spam] sent=" +
                    std::to_string(sent) + " failed=" +
                    std::to_string(failed));
                lastReportAt = utilityService.nowMs();
            }
            utilityService.sleepMs(1);
            continue;
        }

        std::vector<uint8_t> payload;
        if (!loRaTransformer.transform(payloadRaw, payload)) {
            failed++;
            terminalView.println("TX build failed.");
            break;
        }

        firstSend = false;
        lastSendAt = now;
        if (loRaService.send(payload.data(), payload.size())) {
            sent++;
        } else {
            failed++;
            terminalView.println("TX failed: #" +
                std::to_string(sent + failed));
        }

        utilityService.sleepMs(1);
    }

    terminalView.println("\n[Spam summary]");
    terminalView.println("Sent: " +
        std::to_string(sent));
    terminalView.println("Failed: " +
        std::to_string(failed));
    terminalView.println("Duration: " +
        std::to_string(utilityService.nowMs() - startedAt) + " ms");
    terminalView.println("");
}

/*
Transmit a bounded continuous wave for controlled receiver testing
*/
void LoRaController::handleJam(const TerminalCommand& cmd) {
    ensureConfigured();
    if (!configured) return;

    const std::string durationRaw = terminalCommandTransformer.tail(cmd);
    int durationSeconds = 10;
    if (durationRaw.empty()) {
        durationSeconds = userInputManager.readValidatedInt(
            "Continuous wave duration (seconds)", 10, 1, 60);
    } else {
        if (!argTransformer.isValidNumber(durationRaw)) {
            terminalView.println("Usage: jam [1..60 seconds]\n");
            return;
        }
        durationSeconds = static_cast<int>(argTransformer.parseHexOrDec32(durationRaw));
        if (durationSeconds < 1 || durationSeconds > 60) {
            terminalView.println("Usage: jam [1..60 seconds]\n");
            return;
        }
    }

    terminalView.println("\n[WARNING: LoRa continuous wave]");
    terminalView.println("This will occupy the current channel without packets.");
    terminalView.println("Use only in an isolated lab on equipment you control.");
    terminalView.println("Freq: " +
        argTransformer.formatFloat(state.getLoRaFrequency(), 3) + " MHz");
    terminalView.println("Power: " +
        std::to_string(state.getLoRaPower()) + " dBm");
    terminalView.println("Maximum duration: " +
        std::to_string(durationSeconds) + " seconds");

    if (!userInputManager.readYesNo("Start continuous wave?", false)) {
        terminalView.println("LoRa jam: cancelled.\n");
        return;
    }

    if (!loRaService.startContinuousWave()) {
        terminalView.println("LoRa jam: start failed.\n");
        return;
    }

    terminalView.println("LoRa jam: active. Press [ENTER] to stop.\n");
    const uint32_t startedAt = utilityService.nowMs();
    while (utilityService.nowMs() - startedAt < static_cast<uint32_t>(durationSeconds) * 1000u) {
        const char c = terminalInput.readChar();
        if (c == '\r' || c == '\n') break;
        utilityService.sleepMs(1);
    }
    loRaService.stopContinuousWave();

    terminalView.println("\n[Jam summary]");
    terminalView.println("Duration: " +
        std::to_string(utilityService.nowMs() - startedAt) + " ms\n");
}

/*
Receive LoRa packets
*/
void LoRaController::handleReceive() {
    ensureConfigured();
    if (!configured) return;

    terminalView.println("\n[LoRa RX]");
    terminalView.println("Freq: " +
        argTransformer.formatFloat(state.getLoRaFrequency(), 3) +
        " MHz");
    terminalView.println("BW: " +
        std::to_string(state.getLoRaBandwidth()) + " kHz");
    terminalView.println("SF: " +
        std::to_string(state.getLoRaSpreadingFactor()));
    terminalView.println("Stop: press [ENTER]\n");

    if (!loRaService.startReceive(true)) {
        terminalView.println("LoRa RX: start failed.\n");
        return;
    }

    uint32_t packetNumber = 0;
    uint32_t lastPacketAt = 0;
    uint32_t errors = 0;
    const uint32_t droppedAtStart = loRaService.getRxDropped();

    while (true) {
        const char c = terminalInput.readChar();
        if (c == '\r' || c == '\n') break;

        std::vector<uint8_t> payload;
        const int16_t result = loRaService.pollReceive(payload);
        if (result == LoRaService::RECEIVE_ERROR) {
            errors++;
            utilityService.sleepMs(1);
            continue;
        }
        if (result != LoRaService::RECEIVE_OK) {
            utilityService.sleepMs(1);
            continue;
        }

        const uint32_t now = utilityService.nowMs();
        packetNumber++;

        terminalView.println("[RX #" +
            std::to_string(packetNumber) + "]");
        terminalView.println("Time: " +
            std::to_string(now) + " ms");
        if (lastPacketAt != 0) {
            terminalView.println("Gap: " +
                std::to_string(now - lastPacketAt) + " ms");
        }
        terminalView.println("Length: " +
            std::to_string(payload.size()) + " bytes");
        terminalView.println("RSSI: " +
            argTransformer.formatFloat(loRaService.getRssi(), 1) +
            " dBm");
        terminalView.println("SNR: " +
            argTransformer.formatFloat(loRaService.getSnr(), 1) +
            " dB");
        terminalView.println(argTransformer.formatHexAscii(
            payload.data(), payload.size(), true, 8));
        terminalView.println("");
        lastPacketAt = now;
    }

    loRaService.stopReceive();
    const uint32_t droppedNow = loRaService.getRxDropped();
    const uint32_t dropped = droppedNow >= droppedAtStart
        ? droppedNow - droppedAtStart
        : 0;

    terminalView.println("\n[RX summary]");
    terminalView.println("Packets: " +
        std::to_string(packetNumber));
    terminalView.println("Errors: " +
        std::to_string(errors));
    terminalView.println("Dropped: " +
        std::to_string(dropped));
    terminalView.println("");
}

/*
Receive and save a LoRa packet to LittleFS
*/
void LoRaController::handleRecord() {
    ensureConfigured();
    if (!configured) return;
    if (!mountLittleFs()) return;

    constexpr size_t MIN_FREE_BYTES = 2048;
    const size_t freeBytes = littleFsService.freeBytes();
    if (freeBytes < MIN_FREE_BYTES) {
        terminalView.println("LoRa record: LittleFS full.");
        terminalView.println("Need at least 2 KB free.\n");
        return;
    }

    terminalView.println("\n[LoRa record]");
    terminalView.println("Freq: " +
        argTransformer.formatFloat(
            state.getLoRaFrequency(), 3) +
        " MHz");
    terminalView.println("Waiting for one packet.");
    terminalView.println("Stop: press [ENTER]\n");

    if (!loRaService.startReceive(true)) {
        terminalView.println("Record: RX start failed.\n");
        return;
    }

    std::vector<uint8_t> payload;
    uint32_t errors = 0;

    while (payload.empty()) {
        const char c = terminalInput.readChar();
        if (c == '\r' || c == '\n') break;

        const int16_t result = loRaService.pollReceive(payload);
        if (result == LoRaService::RECEIVE_ERROR) {
            errors++;
        }
        if (result != LoRaService::RECEIVE_OK) {
            payload.clear();
            utilityService.sleepMs(1);
        }
    }

    loRaService.stopReceive();

    if (payload.empty()) {
        terminalView.println("Record stopped.");
        terminalView.println("No packet saved.");
        if (errors > 0) {
            terminalView.println("RX errors: " +
                std::to_string(errors));
        }
        terminalView.println("");
        return;
    }

    const LoRaFrame frame = loRaTransformer.fromCapture(
        payload, state.getLoRaProfile(), loRaService.getRssi(), loRaService.getSnr());

    terminalView.println("[Packet captured]");
    terminalView.println("Length: " +
        std::to_string(frame.payload.size()) +
        " bytes");
    terminalView.println("RSSI: " +
        argTransformer.formatFloat(frame.rssi, 1) +
        " dBm");
    terminalView.println("SNR: " +
        argTransformer.formatFloat(frame.snr, 1) +
        " dB");
    terminalView.println(argTransformer.formatHexAscii(
        frame.payload.data(), frame.payload.size(),
        true, 8));

    std::string defaultName =
        "lora_" + std::to_string(utilityService.nowMs() % 1000000);
    std::string fileBase =
        userInputManager.readSanitizedString(
            "File name", defaultName, false);
    if (fileBase.empty()) fileBase = defaultName;

    std::string path = "/" + fileBase;
    if (path.size() < 6 ||
        path.substr(path.size() - 5) != ".lora") {
        path += ".lora";
    }

    const std::string text = loRaTransformer.transformToFileFormat(frame);
    if (!littleFsService.write(path, text)) {
        terminalView.println("LoRa record: write failed.");
        terminalView.println("File: " + path + "\n");
        return;
    }

    terminalView.println("LoRa record: saved.");
    terminalView.println("File: " + path);
    terminalView.println("Use load to send it.\n");
}

/*
Load and send a LoRa packet from LittleFS
*/
void LoRaController::handleLoad() {
    ensureConfigured();
    if (!configured) return;
    if (!mountLittleFs()) return;

    auto files = littleFsService.listFiles("/", ".lora");
    if (files.empty()) {
        terminalView.println("LoRa load: no .lora files.\n");
        return;
    }

    files.emplace_back("Exit");
    terminalView.println("\n[LoRa files]");
    const int fileIndex =
        userInputManager.readValidatedChoiceIndex(
            "File number", files,
            static_cast<int>(files.size() - 1));

    if (fileIndex == static_cast<int>(files.size() - 1)) {
        terminalView.println("LoRa load: cancelled.\n");
        return;
    }

    const std::string filename = files[fileIndex];
    const std::string path = "/" + filename;
    constexpr size_t MAX_FILE_SIZE = 8 * 1024;
    const size_t fileSize = littleFsService.getFileSize(path);

    if (fileSize == 0 || fileSize > MAX_FILE_SIZE) {
        terminalView.println("LoRa load: invalid file size.");
        terminalView.println("File: " + filename + "\n");
        return;
    }

    std::string text;
    if (!littleFsService.readAll(path, text)) {
        terminalView.println("LoRa load: read failed.");
        terminalView.println("File: " + filename + "\n");
        return;
    }

    LoRaFrame frame;
    if (!loRaTransformer.transformFromFileFormat(text, frame)) {
        terminalView.println("LoRa load: invalid .lora file.");
        terminalView.println("File: " + filename + "\n");
        return;
    }

    terminalView.println("\n[LoRa load]");
    terminalView.println("File: " + filename);
    terminalView.println("Length: " +
        std::to_string(frame.payload.size()) +
        " bytes");
    terminalView.println("Freq: " +
        argTransformer.formatFloat(
            frame.profile.frequency, 3) +
        " MHz");
    terminalView.println("BW: " +
        std::to_string(frame.profile.bandwidth) +
        " kHz");
    terminalView.println("SF: " +
        std::to_string(frame.profile.spreadingFactor));
    terminalView.println(argTransformer.formatHexAscii(
        frame.payload.data(), frame.payload.size(),
        true, 8));

    bool profileRestored = false;
    if (loRaService.transmitFrame(frame, profileRestored)) {
        terminalView.println("LoRa load: frame sent.\n");
    } else {
        terminalView.println("LoRa load: send failed.\n");
    }
    if (!profileRestored) terminalView.println("Load: profile restore failed.\n");
}

/*
Monitor the LoRa RSSI level
*/
void LoRaController::handleRssi(const TerminalCommand& cmd) {
    ensureConfigured();
    if (!configured) return;

    const int intervalMs = readOptionalInterval(cmd, "RSSI refresh interval (ms)", 250, 20, 5000);
    if (intervalMs < 0) return;

    terminalView.println("\n[LoRa RSSI]");
    terminalView.println("Freq: " +
        argTransformer.formatFloat(
            state.getLoRaFrequency(), 3) +
        " MHz");
    terminalView.println("Stop: press [ENTER]\n");

    int16_t globalMinimum = std::numeric_limits<int16_t>::max();
    int16_t globalMaximum = std::numeric_limits<int16_t>::min();
    int64_t averageAccumulator = 0;
    uint32_t rounds = 0;

    while (true) {
        const char c = terminalInput.readChar();
        if (c == '\r' || c == '\n') break;

        LoRaService::RssiStats stats;
        const uint32_t sampleDuration = static_cast<uint32_t>(std::min(intervalMs, 200));
        if (loRaService.measureRssi(state.getLoRaFrequency(), sampleDuration, stats)) {
            globalMinimum = std::min(globalMinimum, stats.minimum);
            globalMaximum = std::max(globalMaximum, stats.maximum);
            averageAccumulator += static_cast<int32_t>(std::lround(stats.average * 10.0f));
            rounds++;

            terminalView.println("[RSSI #" +
                std::to_string(rounds) + "]");
            terminalView.println("Min: " +
                std::to_string(stats.minimum) + " dBm");
            terminalView.println("Avg: " +
                argTransformer.formatFloat(
                    stats.average, 1) + " dBm");
            terminalView.println("Max: " +
                std::to_string(stats.maximum) + " dBm");
        }

        const int remaining = intervalMs - static_cast<int>(sampleDuration);
        if (remaining > 0) utilityService.sleepMs(remaining);
    }

    if (rounds > 0) {
        const float globalAverage = static_cast<float>(averageAccumulator) / 10.0f / static_cast<float>(rounds);
        terminalView.println("\n[RSSI summary]");
        terminalView.println("Min: " +
            std::to_string(globalMinimum) + " dBm");
        terminalView.println("Avg: " +
            argTransformer.formatFloat(
                globalAverage, 1) + " dBm");
        terminalView.println("Max: " +
            std::to_string(globalMaximum) + " dBm\n");
    } else {
        terminalView.println("\nRSSI monitor stopped without samples.\n");
    }
}

/*
Convert LoRa RSSI activity to audio tones
*/
void LoRaController::handleEar() {
    ensureConfigured();
    if (!configured) return;

    const int rssiGate = userInputManager.readValidatedInt(
        "RSSI gate (dBm)", -85, -170, 0);

    i2sService.configureOutput(
        state.getI2sBclkPin(), state.getI2sLrckPin(), state.getI2sDataPin(),
        state.getI2sSampleRate(), state.getI2sBitsPerSample(),
        state.getI2sPercentLevel());

    terminalView.println("\n[LoRa ear]");
    terminalView.println("Freq: " +
        argTransformer.formatFloat(state.getLoRaFrequency(), 3) + " MHz");
    terminalView.println("Gate: " + std::to_string(rssiGate) + " dBm");
    terminalView.println("RSSI is mapped to the configured I2S output.");
    terminalView.println("Stop: press [ENTER]\n");

    uint32_t lastReportAt = 0;
    while (true) {
        const char c = terminalInput.readChar();
        if (c == '\r' || c == '\n') break;

        LoRaService::RssiStats stats;
        if (!loRaService.measureRssi(state.getLoRaFrequency(), 20, stats)) {
            utilityService.sleepMs(1);
            continue;
        }

        if (stats.maximum >= rssiGate) {
            const int rssi = static_cast<int>(stats.maximum);
            const int clamped = std::max(-120, std::min(-30, rssi));
            const uint16_t frequencyHz = static_cast<uint16_t>(
                800 + (clamped + 120) * (4000 - 800) / 90);
            i2sService.playTone(state.getI2sSampleRate(), frequencyHz, 15);
        }

        const uint32_t now = utilityService.nowMs();
        if (now - lastReportAt >= 1000) {
            terminalView.println("[Ear] RSSI=" +
                std::to_string(stats.maximum) + " dBm");
            lastReportAt = now;
        }
    }

    i2sService.end();
    terminalView.println("LoRa ear: stopped.\n");
}

/*
Scan a LoRa frequency range
*/
void LoRaController::handleScan() {
    ensureConfigured();
    if (!configured) return;

    const float originalFrequency = state.getLoRaFrequency();
    const float defaultStart =
        std::max(150.0f, originalFrequency - 1.0f);
    const float defaultEnd =
        std::min(960.0f, originalFrequency + 1.0f);

    float start = userInputManager.readValidatedFloat(
        "Start frequency MHz", defaultStart, 150.0f, 960.0f);
    float end = userInputManager.readValidatedFloat(
        "End frequency MHz", defaultEnd, 150.0f, 960.0f);
    if (end < start) std::swap(start, end);

    float step = userInputManager.readValidatedFloat(
        "Step MHz", 0.2f, 0.01f, 20.0f);
    const int dwellMs = userInputManager.readValidatedInt(
        "Dwell per frequency (ms)", 40, 10, 2000);
    const int threshold = userInputManager.readValidatedInt(
        "Peak threshold (dBm)", -85, -170, 0);

    size_t points = static_cast<size_t>(
        std::floor((end - start) / step)) + 1;
    if (points > 200) {
        step = (end - start) / 199.0f;
        points = 200;
        terminalView.println("Scan step adjusted.");
        terminalView.println("Step: " +
            argTransformer.formatFloat(step, 4) +
            " MHz");
    }

    std::vector<float> frequencies;
    frequencies.reserve(points);
    for (size_t i = 0; i < points; ++i) {
        frequencies.push_back(std::min(
            end, start + step * static_cast<float>(i)));
    }

    std::vector<int16_t> best(
        frequencies.size(),
        std::numeric_limits<int16_t>::min());
    std::vector<bool> wasAbove(frequencies.size(), false);

    terminalView.println("\n[LoRa RSSI scan]");
    terminalView.println("Start: " +
        argTransformer.formatFloat(start, 3) +
        " MHz");
    terminalView.println("End: " +
        argTransformer.formatFloat(end, 3) +
        " MHz");
    terminalView.println("Step: " +
        argTransformer.formatFloat(step, 3) +
        " MHz");
    terminalView.println("Threshold: " +
        std::to_string(threshold) + " dBm");
    terminalView.println("Stop: press [ENTER]\n");

    uint32_t sweeps = 0;
    uint32_t samples = 0;
    uint32_t hitEvents = 0;
    bool stopRequested = false;

    while (!stopRequested) {
        sweeps++;

        for (size_t i = 0; i < frequencies.size(); ++i) {
            const char c = terminalInput.readChar();
            if (c == '\r' || c == '\n') {
                stopRequested = true;
                break;
            }

            LoRaService::RssiStats stats;
            if (!loRaService.measureRssi(
                    frequencies[i],
                    static_cast<uint32_t>(dwellMs),
                    stats)) {
                continue;
            }

            samples++;
            best[i] = std::max(best[i], stats.maximum);
            const bool above = stats.maximum >= threshold;

            if (above && !wasAbove[i]) {
                hitEvents++;
                terminalView.println("[HIT]");
                terminalView.println("Freq: " +
                    argTransformer.formatFloat(
                        frequencies[i], 3) +
                    " MHz");
                terminalView.println("Peak: " +
                    std::to_string(stats.maximum) +
                    " dBm");
                terminalView.println("Avg: " +
                    argTransformer.formatFloat(
                        stats.average, 1) +
                    " dBm\n");
                wasAbove[i] = true;
            } else if (stats.maximum < threshold - 2) {
                // Small hysteresis avoids printing the same noisy edge on
                // every pass while still reporting a new activity burst.
                wasAbove[i] = false;
            }
        }

        if (!stopRequested &&
            (sweeps == 1 || sweeps % 10 == 0)) {
            terminalView.println("[SCAN]");
            terminalView.println("Pass: " +
                std::to_string(sweeps));
            terminalView.println("Hits: " +
                std::to_string(hitEvents) + "\n");
        }
    }

    loRaService.setFrequency(originalFrequency);

    std::vector<size_t> ranked(frequencies.size());
    for (size_t i = 0; i < ranked.size(); ++i) ranked[i] = i;
    std::sort(ranked.begin(), ranked.end(),
        [&](size_t a, size_t b) {
            return best[a] > best[b];
        });

    terminalView.println("\n[Scan summary]");
    terminalView.println("Passes: " +
        std::to_string(sweeps));
    terminalView.println("Samples: " +
        std::to_string(samples));
    terminalView.println("Hit events: " +
        std::to_string(hitEvents));

    size_t validHits = 0;
    for (const size_t index : ranked) {
        if (best[index] >= threshold) validHits++;
    }

    if (validHits == 0) {
        terminalView.println("No peak reached threshold.");
        terminalView.println("Original frequency restored.\n");
        return;
    }

    terminalView.println("\n[Best hits]");
    const size_t shown = std::min<size_t>(5, validHits);
    size_t printed = 0;
    for (const size_t index : ranked) {
        if (best[index] < threshold) continue;

        terminalView.println("Freq: " +
            argTransformer.formatFloat(
                frequencies[index], 3) +
            " MHz");
        terminalView.println("Peak: " +
            std::to_string(best[index]) +
            " dBm");
        printed++;
        if (printed >= shown) break;
    }

    const size_t bestIndex = ranked.front();
    const bool saveBest = userInputManager.readYesNo(
        "Save best frequency?", true);

    if (saveBest &&
        loRaService.setFrequency(frequencies[bestIndex])) {
        state.setLoRaFrequency(frequencies[bestIndex]);
        terminalView.println("LoRa frequency saved.");
        terminalView.println("Freq: " +
            argTransformer.formatFloat(
                frequencies[bestIndex], 3) +
            " MHz\n");
    } else {
        loRaService.setFrequency(originalFrequency);
        terminalView.println("Original frequency restored.\n");
    }
}


/*
Display a LoRa frequency waterfall
*/
void LoRaController::handleWaterfall() {
    ensureConfigured();
    if (!configured) return;

    const float originalFrequency = state.getLoRaFrequency();
    const float defaultStart =
        std::max(150.0f, originalFrequency - 1.0f);
    const float defaultEnd =
        std::min(960.0f, originalFrequency + 1.0f);

    float start = userInputManager.readValidatedFloat(
        "Start frequency MHz", defaultStart, 150.0f, 960.0f);
    float end = userInputManager.readValidatedFloat(
        "End frequency MHz", defaultEnd, 150.0f, 960.0f);
    if (end < start) std::swap(start, end);

    float step = userInputManager.readValidatedFloat(
        "Step MHz", 0.1f, 0.01f, 20.0f);
    const int dwellMs = userInputManager.readValidatedInt(
        "Hold per frequency (ms)", 40, 10, 2000);
    const int threshold = userInputManager.readValidatedInt(
        "Peak threshold (dBm)", -85, -170, 0);

    if (std::fabs(end - start) < 0.0001f) {
        end = std::min(960.0f, start + step);
        if (std::fabs(end - start) < 0.0001f) {
            start = std::max(150.0f, end - step);
        }
    }

    size_t points = static_cast<size_t>(
        std::ceil((end - start) / step)) + 1;
    if (points < 2) points = 2;
    if (points > 200) {
        points = 200;
        terminalView.println("Waterfall step adjusted.");
    }
    step = (end - start) /
        static_cast<float>(points - 1);

    std::vector<float> frequencies;
    frequencies.reserve(points);
    for (size_t i = 0; i < points; ++i) {
        frequencies.push_back(
            start + step * static_cast<float>(i));
    }

    terminalView.println("\n[LoRa waterfall]");
    terminalView.println("Start: " +
        argTransformer.formatFloat(start, 3) +
        " MHz");
    terminalView.println("End: " +
        argTransformer.formatFloat(end, 3) +
        " MHz");
    terminalView.println("Step: " +
        argTransformer.formatFloat(step, 3) +
        " MHz");
    terminalView.println("Threshold: " +
        std::to_string(threshold) +
        " dBm");
    terminalView.println("Stop: press [ENTER]\n");

    // Keep enough RSSI range to show the noise floor and strong signals.
    // The visual mapping below deliberately compresses everything below the
    // activity threshold so background noise stays thin and easy to separate
    // from an actual transmission.
    const int dbmMin = std::max(-170, threshold - 30);
    const int dbmMax = std::min(0, threshold + 30);

    int bestDbm = -171;
    float bestFrequency = 0.0f;
    std::string title = "P:--";
    size_t index = 0;

    while (true) {
        const char c = terminalInput.readChar();
        if (c == '\r' || c == '\n') break;

        if (index == 0) {
            // Keep the header stable for a complete sweep: it shows the
            // frequency with the strongest RSSI from the previous pass.
            if (bestDbm > -171) {
                title = "P:" +
                    argTransformer.formatFloat(bestFrequency, 3) +
                    "MHz";
            } else {
                title = "P:--";
            }

            bestDbm = -171;
            bestFrequency = 0.0f;
        }

        LoRaService::RssiStats stats;
        int peak = dbmMin;
        if (loRaService.measureRssi(
                frequencies[index],
                static_cast<uint32_t>(dwellMs),
                stats)) {
            peak = stats.maximum;
            if (peak > bestDbm) {
                bestDbm = peak;
                bestFrequency = frequencies[index];
            }
        }

        const int clamped = std::max(
            dbmMin, std::min(dbmMax, peak));
        constexpr int NOISE_MAX_LEVEL = 18;
        int level = 1;
        if (clamped < threshold && threshold > dbmMin) {
            level = 1 + static_cast<int>(
                static_cast<int64_t>(clamped - dbmMin) *
                (NOISE_MAX_LEVEL - 1) / (threshold - dbmMin));
        } else if (dbmMax > threshold) {
            level = NOISE_MAX_LEVEL + static_cast<int>(
                static_cast<int64_t>(clamped - threshold) *
                (100 - NOISE_MAX_LEVEL) / (dbmMax - threshold));
        }
        level = std::max(1, std::min(100, level));

        deviceView.drawWaterfall(
            title,
            start,
            end,
            "MHz",
            static_cast<int>(index),
            static_cast<int>(frequencies.size()),
            level);

        index++;
        if (index >= frequencies.size()) index = 0;
    }

    loRaService.setFrequency(originalFrequency);
    terminalView.println("LoRa waterfall stopped.");
    terminalView.println("Original frequency restored.\n");
}

/*
Monitor LoRa channel activity detection
*/
void LoRaController::handleCad(const TerminalCommand& cmd) {
    ensureConfigured();
    if (!configured) return;

    const int intervalMs = readOptionalInterval(cmd, "CAD interval (ms)", 250, 20, 5000);
    if (intervalMs < 0) return;

    terminalView.println("\n[LoRa CAD]");
    terminalView.println("Freq: " +
        argTransformer.formatFloat(
            state.getLoRaFrequency(), 3) +
        " MHz");
    terminalView.println("Uses current BW and SF.");
    terminalView.println("Stop: press [ENTER]\n");

    uint32_t checks = 0;
    uint32_t detections = 0;
    uint32_t windowChecks = 0;
    uint32_t windowDetections = 0;
    constexpr uint32_t CAD_REPORT_WINDOW = 10;

    while (true) {
        const char c = terminalInput.readChar();
        if (c == '\r' || c == '\n') break;

        bool detected = false;
        if (loRaService.runCad(detected, 500)) {
            checks++;
            windowChecks++;
            if (detected) {
                detections++;
                windowDetections++;
            }

            if (windowChecks >= CAD_REPORT_WINDOW) {
                terminalView.println("[CAD window]");
                terminalView.println("Checks: " +
                    std::to_string(windowChecks));
                terminalView.println("Activity detected: " +
                    std::to_string(windowDetections));
                windowChecks = 0;
                windowDetections = 0;
            }
        }

        utilityService.sleepMs(intervalMs);
    }

    // Do not discard an incomplete reporting window when the user stops.
    if (windowChecks > 0) {
        terminalView.println("\n[Last CAD window]");
        terminalView.println("Checks: " +
            std::to_string(windowChecks));
        terminalView.println("Activity detected: " +
            std::to_string(windowDetections));
    }

    terminalView.println("\n[CAD summary]");
    terminalView.println("Checks: " +
        std::to_string(checks));
    terminalView.println("Detected: " +
        std::to_string(detections) + "\n");
}

/*
Calculate the airtime of a LoRa payload
*/
void LoRaController::handleAirtime(const TerminalCommand& cmd) {
    ensureConfigured();
    if (!configured) return;

    std::string value = terminalCommandTransformer.tail(cmd);
    int bytes = 16;
    if (value.empty()) {
        bytes = userInputManager.readValidatedInt("Payload length (1..255 bytes)", bytes, 1, 255);
    } else {
        if (!argTransformer.isValidNumber(value)) {
            terminalView.println("Usage: airtime [1..255]\n");
            return;
        }
        bytes = static_cast<int>(argTransformer.parseHexOrDec32(value));
        if (bytes < 1 || bytes > 255) {
            terminalView.println("Usage: airtime [1..255]\n");
            return;
        }
    }

    const uint32_t airtime = loRaService.getTimeOnAir(static_cast<size_t>(bytes));
    terminalView.println("\n[LoRa airtime]");
    terminalView.println("Length: " +
        std::to_string(bytes) + " bytes");
    terminalView.println("Time: " +
        std::to_string(airtime) + " ms");
    printCurrentProfile("[Radio profile]");
    terminalView.println("");
}

/*
Set the LoRa operating frequency
*/
void LoRaController::handleSetFrequency(const TerminalCommand& cmd) {
    float frequency = state.getLoRaFrequency();
    const std::string value = terminalCommandTransformer.tail(cmd);

    if (value.empty()) {
        frequency = userInputManager.readValidatedFloat("Frequency MHz", frequency, 150.0f, 960.0f);
    } else {
        if (!argTransformer.isValidFloat(value)) {
            terminalView.println("Usage: setfreq <150..960 MHz>\n");
            return;
        }
        frequency = std::stof(value);
        if (frequency < 150.0f || frequency > 960.0f) {
            terminalView.println("Usage: setfreq <150..960 MHz>\n");
            return;
        }
    }

    ensureConfigured();
    if (!configured) return;

    if (!loRaService.setFrequency(frequency)) {
        terminalView.println("LoRa: frequency rejected.\n");
        return;
    }

    state.setLoRaFrequency(frequency);
    terminalView.println("LoRa: frequency saved.");
    terminalView.println("Freq: " +
        argTransformer.formatFloat(frequency, 3) +
        " MHz\n");
}

/*
Display the LoRa radio status
*/
void LoRaController::handleStatus() {
    terminalView.println("\n[LoRa SX1262]");
    terminalView.println("State: " +
        std::string(loRaService.isInitialized()
            ? "ready"
            : "released"));
    terminalView.println("RX active: " +
        std::string(loRaService.isReceiving()
            ? "yes"
            : "no"));

    printCurrentProfile("[Radio profile]");

    terminalView.println("[SPI pins]");
    terminalView.println("SCK: " +
        std::to_string(state.getLoRaSckPin()));
    terminalView.println("MISO: " +
        std::to_string(state.getLoRaMisoPin()));
    terminalView.println("MOSI: " +
        std::to_string(state.getLoRaMosiPin()));
    terminalView.println("CS: " +
        std::to_string(state.getLoRaCsPin()));

    terminalView.println("[Control pins]");
    terminalView.println("RESET: " +
        std::to_string(state.getLoRaRstPin()));
    terminalView.println("BUSY: " +
        std::to_string(state.getLoRaBusyPin()));
    terminalView.println("DIO1: " +
        std::to_string(state.getLoRaDio1Pin()));
    terminalView.println("TCXO: " +
        argTransformer.formatFloat(
            state.getLoRaTcxoVoltage(), 1) +
        " V");

    terminalView.println("[Counters]");
    terminalView.println("TX: " +
        std::to_string(loRaService.getTxPackets()));
    terminalView.println("TX errors: " +
        std::to_string(loRaService.getTxErrors()));
    terminalView.println("RX: " +
        std::to_string(loRaService.getRxPackets()));
    terminalView.println("RX errors: " +
        std::to_string(loRaService.getRxErrors()));
    terminalView.println("RX timeouts: " +
        std::to_string(loRaService.getRxTimeouts()));
    terminalView.println("RX dropped: " +
        std::to_string(loRaService.getRxDropped()));

    if (loRaService.getLastPacketLength() > 0) {
        terminalView.println("[Last RX]");
        terminalView.println("Length: " +
            std::to_string(
                loRaService.getLastPacketLength()) +
            " bytes");
        terminalView.println("RSSI: " +
            argTransformer.formatFloat(
                loRaService.getRssi(), 1) +
            " dBm");
        terminalView.println("SNR: " +
            argTransformer.formatFloat(
                loRaService.getSnr(), 1) +
            " dB");
    }

    terminalView.println("");
}

/*
Mount LittleFS for LoRa packet storage
*/
bool LoRaController::mountLittleFs() {
    if (littleFsService.mounted()) return true;

    if (!littleFsService.begin()) {
        terminalView.println("LoRa: LittleFS mount failed.\n");
        return false;
    }

    terminalView.println("LoRa: LittleFS mounted.");
    return true;
}

/*
Configure the SX1262 pins
*/
void LoRaController::configurePins() {
    auto forbidden = state.getProtectedPins();
    auto readPin = [&](const char* label, uint8_t current) {
        const uint8_t pin = userInputManager.readValidatedPinNumber(label, current, forbidden);
        forbidden.push_back(pin);
        return pin;
    };

    state.setLoRaSckPin(readPin("SX1262 SCK GPIO", state.getLoRaSckPin()));
    state.setLoRaMisoPin(readPin("SX1262 MISO GPIO", state.getLoRaMisoPin()));
    state.setLoRaMosiPin(readPin("SX1262 MOSI GPIO", state.getLoRaMosiPin()));
    state.setLoRaCsPin(readPin("SX1262 CS GPIO", state.getLoRaCsPin()));
    state.setLoRaRstPin(readPin("SX1262 RESET GPIO", state.getLoRaRstPin()));
    state.setLoRaBusyPin(readPin("SX1262 BUSY GPIO", state.getLoRaBusyPin()));
    state.setLoRaDio1Pin(readPin("SX1262 DIO1 GPIO", state.getLoRaDio1Pin()));
}

/*
Configure the LoRa radio profile
*/
void LoRaController::configureRadioProfile() {
    state.setLoRaFrequency(userInputManager.readValidatedFloat(
        "\n\rFrequency MHz", state.getLoRaFrequency(), 150.0f, 960.0f));

    const std::vector<int> bandwidths = {125, 250, 500};
    int bandwidthIndex = 0;
    for (size_t i = 0; i < bandwidths.size(); ++i) {
        if (bandwidths[i] == state.getLoRaBandwidth()) bandwidthIndex = static_cast<int>(i);
    }
    const int selectedBandwidth = userInputManager.readValidatedChoiceIndex(
        "Bandwidth kHz:", bandwidths, bandwidthIndex);
    state.setLoRaBandwidth(static_cast<uint16_t>(bandwidths[selectedBandwidth]));

    state.setLoRaSpreadingFactor(static_cast<uint8_t>(userInputManager.readValidatedInt(
        "Spreading factor (6..12)", state.getLoRaSpreadingFactor(), 6, 12)));
    state.setLoRaCodingRate(static_cast<uint8_t>(userInputManager.readValidatedInt(
        "Coding rate denominator (5..8)", state.getLoRaCodingRate(), 5, 8)));
    state.setLoRaPower(static_cast<int8_t>(userInputManager.readValidatedInt(
        "TX power dBm (-9..22)", state.getLoRaPower(), -9, 22)));
    state.setLoRaPreambleLength(static_cast<uint16_t>(userInputManager.readValidatedInt(
        "Preamble symbols (6..65535)", state.getLoRaPreambleLength(), 6, 65535)));
    state.setLoRaSyncWord(userInputManager.readValidatedUint16(
        "Sync word", state.getLoRaSyncWord(), true));
    state.setLoRaCrc(userInputManager.readYesNo("Enable CRC?", state.getLoRaCrc()));
    state.setLoRaInvertIq(userInputManager.readYesNo("Invert IQ?", state.getLoRaInvertIq()));

    const std::vector<float> tcxoVoltages = {0.0f, 1.6f, 1.7f, 1.8f, 2.2f, 2.4f, 2.7f, 3.0f, 3.3f};
    int tcxoIndex = 0;
    float bestDistance = std::numeric_limits<float>::max();
    for (size_t i = 0; i < tcxoVoltages.size(); ++i) {
        const float distance = std::fabs(tcxoVoltages[i] - state.getLoRaTcxoVoltage());
        if (distance < bestDistance) {
            bestDistance = distance;
            tcxoIndex = static_cast<int>(i);
        }
    }
    const int selectedTcxo = userInputManager.readValidatedChoiceIndex(
        "TCXO voltage (0 = XTAL):", tcxoVoltages, tcxoIndex);
    state.setLoRaTcxoVoltage(tcxoVoltages[selectedTcxo]);
}

/*
Reset the LoRa radio profile to default values
*/
void LoRaController::resetDefaultRadioProfile() {
    state.setLoRaFrequency(868.0f);
    state.setLoRaBandwidth(125);
    state.setLoRaSpreadingFactor(9);
    state.setLoRaCodingRate(7);
    state.setLoRaPower(14);
    state.setLoRaPreambleLength(8);
    state.setLoRaSyncWord(0x1424);
    state.setLoRaCrc(true);
    state.setLoRaInvertIq(false);
    state.setLoRaTcxoVoltage(1.8f);
}

/*
Display the current LoRa radio profile
*/
void LoRaController::printCurrentProfile(const char* title) {
    if (title && title[0] != '\0') {
        terminalView.println(title);
    }
    terminalView.println("Freq: " +
        argTransformer.formatFloat(
            state.getLoRaFrequency(), 3) +
        " MHz");
    terminalView.println("BW: " +
        std::to_string(state.getLoRaBandwidth()) +
        " kHz");
    terminalView.println("SF: " +
        std::to_string(
            state.getLoRaSpreadingFactor()));
    terminalView.println("CR: 4/" +
        std::to_string(state.getLoRaCodingRate()));
    terminalView.println("Power: " +
        std::to_string(state.getLoRaPower()) +
        " dBm");
    terminalView.println("Preamble: " +
        std::to_string(
            state.getLoRaPreambleLength()));
    terminalView.println("Sync: 0x" +
        argTransformer.toHex(
            state.getLoRaSyncWord(), 4));
    terminalView.println("CRC: " +
        std::string(state.getLoRaCrc()
            ? "on"
            : "off"));
    terminalView.println("IQ: " +
        std::string(state.getLoRaInvertIq()
            ? "inverted"
            : "normal"));
}

/*
Read and validate an optional command interval
*/
int LoRaController::readOptionalInterval(const TerminalCommand& cmd, const std::string& label,
                                         int defaultValue, int minimum, int maximum) {
    const std::string value = terminalCommandTransformer.tail(cmd);
    if (value.empty()) {
        return userInputManager.readValidatedInt(label, defaultValue, minimum, maximum);
    }

    if (!argTransformer.isValidNumber(value)) {
        terminalView.println("Invalid interval.\n");
        return -1;
    }

    const uint32_t parsed = argTransformer.parseHexOrDec32(value);
    if (parsed < static_cast<uint32_t>(minimum) || parsed > static_cast<uint32_t>(maximum)) {
        terminalView.println("Interval must be " + std::to_string(minimum) + ".." +
                             std::to_string(maximum) + " ms.\n");
        return -1;
    }
    return static_cast<int>(parsed);
}
