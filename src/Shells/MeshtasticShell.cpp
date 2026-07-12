#include "Shells/MeshtasticShell.h"

#include <algorithm>
#include <esp_system.h>

MeshtasticShell::MeshtasticShell(ITerminalView& terminalView,
                                 IInput& terminalInput,
                                 UserInputManager& userInputManager,
                                 ArgTransformer& argTransformer,
                                 LoRaService& loRaService,
                                 MeshtasticService& meshtasticService)
    : terminalView(terminalView),
      terminalInput(terminalInput),
      userInputManager(userInputManager),
      argTransformer(argTransformer),
      loRaService(loRaService),
      meshtasticService(meshtasticService) {}

void MeshtasticShell::run() {
    if (!loRaService.isInitialized()) {
        terminalView.println("Meshtastic: LoRa is not ready.\n");
        return;
    }

    const LoRaRadioProfile original = loRaService.getProfile();
    frequency_ = original.frequency;

    const uint64_t mac = ESP.getEfuseMac();
    nodeNumber_ = static_cast<uint32_t>(mac);
    if (nodeNumber_ == 0 || nodeNumber_ == 0xFFFFFFFFu) {
        nodeNumber_ = esp_random();
        if (nodeNumber_ == 0 || nodeNumber_ == 0xFFFFFFFFu) {
            nodeNumber_ = 1;
        }
    }
    if (nextPacketId_ == 0) {
        nextPacketId_ = esp_random();
        if (nextPacketId_ == 0) nextPacketId_ = 1;
    }

    if (!MeshtasticService::profileFor(
            presetName_, frequency_, profile_) ||
        !applyProfile()) {
        loRaService.setFrequency(original.frequency);
        loRaService.setModemProfile(original);
        terminalView.println("Meshtastic: profile setup failed.\n");
        return;
    }

    terminalView.println("\n=== Meshtastic Shell ===");
    terminalView.println("Analysis shell over LoRa.");
    terminalView.println("Raw LoRa profile is restored on exit.\n");

    while (true) {
        const int index = userInputManager.readValidatedChoiceIndex(
            "Meshtastic action", kActions, kActionCount, 0);

        if (index < 0 || index == 8) break;

        switch (index) {
            case 0: cmdStatus(); break;
            case 1: cmdPreset(); break;
            case 2: cmdFrequency(); break;
            case 3: cmdSetKey(); break;
            case 4: cmdClearKey(); break;
            case 5: cmdSend(); break;
            case 6: cmdReceive(); break;
            case 7: cmdInspect(); break;
            default: break;
        }
        terminalView.println("");
    }

    loRaService.stopReceive();
    const bool restored =
        loRaService.setFrequency(original.frequency) &&
        loRaService.setModemProfile(original);

    terminalView.println(restored
        ? "Meshtastic: LoRa profile restored.\n"
        : "Meshtastic: profile restore failed.\n");
}

bool MeshtasticShell::applyProfile() {
    loRaService.stopReceive();
    return loRaService.setFrequency(profile_.frequency) &&
           loRaService.setModemProfile(profile_);
}

void MeshtasticShell::cmdStatus() {
    terminalView.println("\n[Meshtastic status]");
    terminalView.println("Preset: " + presetName_);
    terminalView.println("Freq: " +
        argTransformer.formatFloat(frequency_, 3) + " MHz");
    terminalView.println("BW: " +
        std::to_string(profile_.bandwidth) + " kHz");
    terminalView.println("SF: " +
        std::to_string(profile_.spreadingFactor));
    terminalView.println("CR: 4/" +
        std::to_string(profile_.codingRate));
    terminalView.println("Preamble: " +
        std::to_string(profile_.preambleLength));
    terminalView.println("Header: explicit");
    terminalView.println("CRC: " +
        std::string(profile_.crc ? "on" : "off"));
    terminalView.println("IQ: " +
        std::string(profile_.invertIq
            ? "inverted" : "normal"));
    terminalView.println("Sync reg: 0x" +
        argTransformer.toHex(profile_.syncWord, 4));
    terminalView.println("Mesh sync: 0x2B");
    terminalView.println("Node: " + nodeId(nodeNumber_));
    terminalView.println("Channel: " + channelName_);
    terminalView.println("Hash: 0x" +
        argTransformer.toHex(
            meshtasticService.channelHash(channelName_), 2));
    terminalView.println("Key: " +
        std::string(meshtasticService.hasKey()
            ? std::to_string(meshtasticService.keyBits()) + " bit"
            : "not set"));
}

void MeshtasticShell::cmdPreset() {
    size_t count = 0;
    const MeshtasticService::Preset* presets =
        MeshtasticService::presets(count);

    std::vector<std::string> choices;
    std::vector<size_t> indices;
    int defaultIndex = 0;

    for (size_t i = 0; i < count; ++i) {
        // LoRaService currently maps 125, 250 and 500 kHz profiles.
        if (presets[i].bandwidth < 125) continue;

        if (presetName_ == presets[i].name) {
            defaultIndex = static_cast<int>(choices.size());
        }

        choices.push_back(std::string(presets[i].name) +
            "  BW" + std::to_string(presets[i].bandwidth) +
            " SF" + std::to_string(presets[i].sf));
        indices.push_back(i);
    }

    if (choices.empty()) {
        terminalView.println("Meshtastic: no supported preset.");
        return;
    }

    const int selected = userInputManager.readValidatedChoiceIndex(
        "Select preset", choices, defaultIndex);
    if (selected < 0 || selected >= static_cast<int>(indices.size())) return;

    const MeshtasticService::Preset& preset = presets[indices[selected]];
    LoRaRadioProfile next;
    if (!MeshtasticService::profileFor(
            preset.name, frequency_, next)) {
        terminalView.println("Meshtastic: invalid preset.");
        return;
    }


    const LoRaRadioProfile previous = profile_;
    profile_ = next;
    if (!applyProfile()) {
        profile_ = previous;
        applyProfile();
        terminalView.println("Meshtastic: preset failed.");
        return;
    }

    presetName_ = preset.name;
    channelName_ = channelNameForPreset(presetName_);
    terminalView.println("Meshtastic preset: " + presetName_);
}

void MeshtasticShell::cmdFrequency() {
    const float nextFrequency = userInputManager.readValidatedFloat(
        "Frequency MHz", frequency_, 150.0f, 960.0f);

    const float previousFrequency = frequency_;
    frequency_ = nextFrequency;
    profile_.frequency = nextFrequency;

    if (!applyProfile()) {
        frequency_ = previousFrequency;
        profile_.frequency = previousFrequency;
        applyProfile();
        terminalView.println("Meshtastic: frequency failed.");
        return;
    }

    terminalView.println("Meshtastic frequency: " +
        argTransformer.formatFloat(frequency_, 3) + " MHz");
}

void MeshtasticShell::cmdSetKey() {
    terminalView.println("\n[Channel key]");
    terminalView.println("Paste a Meshtastic PSK.");
    terminalView.println("Base64 or hex is accepted.");
    terminalView.println("AQ== selects the public key.");

    const std::string key = userInputManager.readString(
        "Key", "");
    if (key.empty()) {
        terminalView.println("Meshtastic: key unchanged.");
        return;
    }

    if (!meshtasticService.setKey(key)) {
        terminalView.println("Meshtastic: invalid key.");
        terminalView.println("Use AQ==, Base64, or 32/64 hex digits.");
        return;
    }

    terminalView.println("Meshtastic: channel key set.");
}

void MeshtasticShell::cmdClearKey() {
    meshtasticService.clearKey();
    terminalView.println("Meshtastic: channel key cleared.");
}

void MeshtasticShell::cmdSend() {
    terminalView.println("\n[Meshtastic TX]");
    const std::string text = userInputManager.readString(
        "Text", "ping");
    if (text.empty()) {
        terminalView.println("Meshtastic TX: cancelled.");
        return;
    }

    const uint32_t packetId = nextPacketId_++;
    if (nextPacketId_ == 0) nextPacketId_ = 1;

    std::vector<uint8_t> frame;
    std::string error;
    if (!meshtasticService.buildTextFrame(
            text, channelName_, nodeNumber_, packetId,
            frame, error)) {
        terminalView.println("Meshtastic TX: " + error);
        return;
    }

    terminalView.println("From: " + nodeId(nodeNumber_));
    terminalView.println("To: broadcast");
    terminalView.println("ID: 0x" +
        argTransformer.toHex(packetId, 8));
    terminalView.println("Channel: 0x" +
        argTransformer.toHex(
            meshtasticService.channelHash(channelName_), 2));
    terminalView.println("Mode: " +
        std::string(meshtasticService.hasKey()
            ? "encrypted" : "clear"));
    printWrapped("Text", text);
    terminalView.println("Length: " +
        std::to_string(frame.size()) + " bytes");

    if (!loRaService.send(frame.data(), frame.size())) {
        terminalView.println("Meshtastic TX: failed.");
        return;
    }
    terminalView.println("Meshtastic TX: sent.");
}

void MeshtasticShell::cmdReceive() {
    terminalView.println("\n[Meshtastic RX]");
    terminalView.println("Preset: " + presetName_);
    terminalView.println("Freq: " +
        argTransformer.formatFloat(frequency_, 3) + " MHz");
    terminalView.println("Stop: press [ENTER]\n");

    if (!loRaService.startReceive(true)) {
        terminalView.println("Meshtastic RX: start failed.");
        return;
    }

    uint32_t received = 0;
    uint32_t parsed = 0;
    uint32_t errors = 0;

    while (true) {
        const char c = terminalInput.readChar();
        if (c == '\r' || c == '\n') break;

        std::vector<uint8_t> frame;
        const int16_t result = loRaService.pollReceive(frame);
        if (result == LoRaService::RECEIVE_ERROR) {
            errors++;
            delay(1);
            continue;
        }
        if (result != LoRaService::RECEIVE_OK) {
            delay(1);
            continue;
        }

        received++;
        MeshtasticPacket packet;
        std::string error;
        if (!meshtasticService.inspect(frame, packet, error)) {
            terminalView.println("[Mesh RX]");
            terminalView.println("Parse: " + error);
            terminalView.println("Length: " +
                std::to_string(frame.size()) + " bytes");
            terminalView.println(argTransformer.formatHexAscii(
                frame.data(), frame.size(), true, 8));
            terminalView.println("");
            continue;
        }

        parsed++;
        printPacket(packet, frame, received);
    }

    loRaService.stopReceive();
    terminalView.println("\n[Meshtastic summary]");
    terminalView.println("Received: " + std::to_string(received));
    terminalView.println("Parsed: " + std::to_string(parsed));
    terminalView.println("RX errors: " + std::to_string(errors));
}

void MeshtasticShell::cmdInspect() {
    terminalView.println("\n[Inspect frame]");
    terminalView.println("Paste the complete radio frame.");

    const std::string raw = userInputManager.readValidatedHexString(
        "Frame ", 0, true);
    const std::vector<uint8_t> frame =
        argTransformer.parseHexList(raw);

    MeshtasticPacket packet;
    std::string error;
    if (!meshtasticService.inspect(frame, packet, error)) {
        terminalView.println("Meshtastic: " + error);
        return;
    }

    printPacket(packet, frame);
}

void MeshtasticShell::printPacket(const MeshtasticPacket& packet,
                                  const std::vector<uint8_t>& frame,
                                  uint32_t index) {
    terminalView.println(index == 0
        ? "\n[Meshtastic packet]"
        : "\n[Mesh #" + std::to_string(index) + "]");

    terminalView.println("From: " + nodeId(packet.source));
    terminalView.println("To: " + nodeId(packet.destination));
    terminalView.println("ID: 0x" +
        argTransformer.toHex(packet.id, 8));
    terminalView.println("Channel: 0x" +
        argTransformer.toHex(packet.channel, 2));
    terminalView.println("Hop: " +
        std::to_string(packet.hopLimit) + "/" +
        std::to_string(packet.hopStart));
    terminalView.println("Ack: " +
        std::string(packet.wantAck ? "yes" : "no"));
    terminalView.println("MQTT: " +
        std::string(packet.viaMqtt ? "yes" : "no"));
    if (index != 0) {
        terminalView.println("RSSI: " +
            argTransformer.formatFloat(loRaService.getRssi(), 1) +
            " dBm");
        terminalView.println("SNR: " +
            argTransformer.formatFloat(loRaService.getSnr(), 1) +
            " dB");
    }

    if (packet.encrypted) {
        terminalView.println("Payload: encrypted");
        if (meshtasticService.hasKey()) {
            terminalView.println("Key: no matching channel");
            terminalView.println("Or packet uses PKC.");
        }
    } else {
        terminalView.println("Type: " + packet.type);
        terminalView.println("Port: " +
            std::to_string(packet.portNum));
        if (!packet.text.empty()) {
            printWrapped("Text", packet.text);
        }
    }

    terminalView.println("Raw length: " +
        std::to_string(frame.size()) + " bytes");
    terminalView.println(argTransformer.formatHexAscii(
        frame.data(), frame.size(), true, 8));
}

void MeshtasticShell::printWrapped(const std::string& label,
                                   const std::string& value,
                                   size_t width) {
    terminalView.println(label + ":");
    if (value.empty()) {
        terminalView.println("  <empty>");
        return;
    }

    size_t offset = 0;
    while (offset < value.size()) {
        size_t count = std::min(width, value.size() - offset);
        size_t end = offset + count;

        if (end < value.size()) {
            const size_t space = value.rfind(' ', end);
            if (space != std::string::npos && space > offset) {
                end = space;
            }
        }

        terminalView.println("  " + value.substr(offset, end - offset));
        offset = end;
        while (offset < value.size() && value[offset] == ' ') offset++;
    }
}

std::string MeshtasticShell::nodeId(uint32_t value) {
    if (value == 0xFFFFFFFFu) return "broadcast";
    return "!" + argTransformer.toHex(value, 8);
}


std::string MeshtasticShell::channelNameForPreset(
        const std::string& preset) {
    if (preset == "LONG_FAST") return "LongFast";
    if (preset == "LONG_SLOW") return "LongSlow";
    if (preset == "VERY_LONG_SLOW") return "VeryLongSlow";
    if (preset == "MEDIUM_SLOW") return "MediumSlow";
    if (preset == "MEDIUM_FAST") return "MediumFast";
    if (preset == "SHORT_SLOW") return "ShortSlow";
    if (preset == "SHORT_FAST") return "ShortFast";
    if (preset == "SHORT_TURBO") return "ShortTurbo";
    return preset;
}
