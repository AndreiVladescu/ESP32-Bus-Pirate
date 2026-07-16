#include "Transformers/LoRaTransformer.h"

#include <cerrno>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <sstream>

namespace {
std::string trim(const std::string& value) {
    size_t begin = 0;
    while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin]))) ++begin;
    size_t end = value.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1]))) --end;
    return value.substr(begin, end - begin);
}

bool parseLong(const std::string& value, long minimum, long maximum, long& result) {
    errno = 0;
    char* end = nullptr;
    const long parsed = std::strtol(value.c_str(), &end, 0);
    while (end && *end != '\0' && std::isspace(static_cast<unsigned char>(*end))) ++end;
    if (errno != 0 || end == value.c_str() || (end && *end != '\0') ||
        parsed < minimum || parsed > maximum) return false;
    result = parsed;
    return true;
}

bool parseFloat(const std::string& value, float minimum, float maximum, float& result) {
    errno = 0;
    char* end = nullptr;
    const float parsed = std::strtof(value.c_str(), &end);
    while (end && *end != '\0' && std::isspace(static_cast<unsigned char>(*end))) ++end;
    if (errno != 0 || end == value.c_str() || (end && *end != '\0') ||
        !std::isfinite(parsed) || parsed < minimum || parsed > maximum) return false;
    result = parsed;
    return true;
}

std::string decodeEscapes(const std::string& input) {
    std::string output;
    output.reserve(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] != '\\' || i + 1 >= input.size()) {
            output.push_back(input[i]);
            continue;
        }
        const char escaped = input[++i];
        switch (escaped) {
            case 'n': output.push_back('\n'); break;
            case 'r': output.push_back('\r'); break;
            case 't': output.push_back('\t'); break;
            case '0': output.push_back('\0'); break;
            case '\\': output.push_back('\\'); break;
            case 'x': {
                if (i + 2 >= input.size() ||
                    !std::isxdigit(static_cast<unsigned char>(input[i + 1])) ||
                    !std::isxdigit(static_cast<unsigned char>(input[i + 2]))) {
                    output += "\\x";
                    break;
                }
                const std::string hex = input.substr(i + 1, 2);
                output.push_back(static_cast<char>(std::strtoul(hex.c_str(), nullptr, 16)));
                i += 2;
                break;
            }
            default: output.push_back('\\'); output.push_back(escaped); break;
        }
    }
    return output;
}
}

LoRaTransformer::LoRaTransformer(IUtilityService& utilityService)
    : utilityService(utilityService) {}

bool LoRaTransformer::transform(const std::string& raw,
                                std::vector<uint8_t>& payload) const {
    payload.clear();
    if (raw.empty()) return false;

    if (raw.size() < 5 || raw.rfind("hex{", 0) != 0 || raw.back() != '}') {
        const std::string text = decodeEscapes(raw);
        payload.assign(text.begin(), text.end());
        return !payload.empty();
    }

    std::istringstream tokens(raw.substr(4, raw.size() - 5));
    std::string token;
    while (tokens >> token) {
        if (token == "??") {
            payload.push_back(static_cast<uint8_t>(utilityService.randomRange(0, 256)));
            continue;
        }
        if (token.size() == 4 && token[0] == '0' && (token[1] == 'x' || token[1] == 'X')) {
            token = token.substr(2);
        }
        if (token.size() != 2 ||
            !std::isxdigit(static_cast<unsigned char>(token[0])) ||
            !std::isxdigit(static_cast<unsigned char>(token[1]))) {
            payload.clear();
            return false;
        }
        payload.push_back(static_cast<uint8_t>(std::strtoul(token.c_str(), nullptr, 16)));
    }
    return !payload.empty();
}

std::string LoRaTransformer::transformToFileFormat(const LoRaFrame& frame) const {
    std::ostringstream out;
    out << "Filetype: ESP32-Bit-Pirate LoRa\nVersion: 1\n";
    out << std::fixed << std::setprecision(6);
    out << "FrequencyMHz: " << frame.profile.frequency << "\n";
    out << "BandwidthKHz: " << frame.profile.bandwidth << "\n";
    out << "SpreadingFactor: " << static_cast<unsigned>(frame.profile.spreadingFactor) << "\n";
    out << "CodingRate: " << static_cast<unsigned>(frame.profile.codingRate) << "\n";
    out << "PowerDbm: " << static_cast<int>(frame.profile.power) << "\n";
    out << "Preamble: " << frame.profile.preambleLength << "\n";
    out << "SyncWord: 0x" << std::uppercase << std::hex << std::setw(4)
        << std::setfill('0') << frame.profile.syncWord << std::dec << "\n";
    out << std::fixed << std::setprecision(1);
    out << "TcxoVoltage: " << frame.profile.tcxoVoltage << "\n";
    out << "CRC: " << (frame.profile.crc ? 1 : 0) << "\n";
    out << "InvertIQ: " << (frame.profile.invertIq ? 1 : 0) << "\n";
    out << std::fixed << std::setprecision(1);
    out << "RSSI: " << frame.rssi << "\nSNR: " << frame.snr << "\n";
    out << "PayloadLength: " << frame.payload.size() << "\nPayloadHex:";
    out << std::uppercase << std::hex << std::setfill('0');
    for (uint8_t value : frame.payload) out << " " << std::setw(2) << static_cast<unsigned>(value);
    out << "\n";
    return out.str();
}

bool LoRaTransformer::transformFromFileFormat(const std::string& text, LoRaFrame& frame) const {
    LoRaFrame parsed;
    size_t expectedLength = 0;
    uint32_t fields = 0;
    constexpr uint32_t REQUIRED_FIELDS = 0x3FFF;
    std::istringstream lines(text);
    std::string line;
    while (std::getline(lines, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const size_t separator = line.find(':');
        if (separator == std::string::npos) continue;
        const std::string key = trim(line.substr(0, separator));
        const std::string value = trim(line.substr(separator + 1));
        long integer = 0;
        if (key == "Filetype") { if (value != "ESP32-Bit-Pirate LoRa") return false; fields |= 1u << 0; }
        else if (key == "Version") { if (!parseLong(value, 1, 1, integer)) return false; fields |= 1u << 1; }
        else if (key == "FrequencyMHz") { if (!parseFloat(value, 150.0f, 960.0f, parsed.profile.frequency)) return false; fields |= 1u << 2; }
        else if (key == "BandwidthKHz") { if (!parseLong(value, 125, 500, integer)) return false; parsed.profile.bandwidth = static_cast<uint16_t>(integer); if (integer != 125 && integer != 250 && integer != 500) return false; fields |= 1u << 3; }
        else if (key == "SpreadingFactor") { if (!parseLong(value, 6, 12, integer)) return false; parsed.profile.spreadingFactor = static_cast<uint8_t>(integer); fields |= 1u << 4; }
        else if (key == "CodingRate") { if (!parseLong(value, 5, 8, integer)) return false; parsed.profile.codingRate = static_cast<uint8_t>(integer); fields |= 1u << 5; }
        else if (key == "PowerDbm") { if (!parseLong(value, -9, 22, integer)) return false; parsed.profile.power = static_cast<int8_t>(integer); fields |= 1u << 6; }
        else if (key == "Preamble") { if (!parseLong(value, 6, 65535, integer)) return false; parsed.profile.preambleLength = static_cast<uint16_t>(integer); fields |= 1u << 7; }
        else if (key == "SyncWord") { if (!parseLong(value, 0, 65535, integer)) return false; parsed.profile.syncWord = static_cast<uint16_t>(integer); fields |= 1u << 8; }
        else if (key == "TcxoVoltage") { if (!parseFloat(value, 0.0f, 3.3f, parsed.profile.tcxoVoltage)) return false; fields |= 1u << 13; }
        else if (key == "CRC") { if (!parseLong(value, 0, 1, integer)) return false; parsed.profile.crc = integer != 0; fields |= 1u << 9; }
        else if (key == "InvertIQ") { if (!parseLong(value, 0, 1, integer)) return false; parsed.profile.invertIq = integer != 0; fields |= 1u << 10; }
        else if (key == "RSSI") { if (!parseFloat(value, -200.0f, 50.0f, parsed.rssi)) return false; }
        else if (key == "SNR") { if (!parseFloat(value, -100.0f, 100.0f, parsed.snr)) return false; }
        else if (key == "PayloadLength") { if (!parseLong(value, 1, 255, integer)) return false; expectedLength = static_cast<size_t>(integer); fields |= 1u << 11; }
        else if (key == "PayloadHex") {
            std::istringstream bytes(value); std::string token;
            while (bytes >> token) {
                if (token.size() != 2 || !std::isxdigit(static_cast<unsigned char>(token[0])) || !std::isxdigit(static_cast<unsigned char>(token[1]))) return false;
                char* byteEnd = nullptr;
                const unsigned long byteValue = std::strtoul(token.c_str(), &byteEnd, 16);
                if (!byteEnd || *byteEnd != '\0' || byteValue > 0xFF) return false;
                parsed.payload.push_back(static_cast<uint8_t>(byteValue));
                if (parsed.payload.size() > 255) return false;
            }
            if (parsed.payload.empty()) return false;
            fields |= 1u << 12;
        }
    }
    if ((fields & REQUIRED_FIELDS) != REQUIRED_FIELDS || parsed.payload.size() != expectedLength) return false;
    frame = std::move(parsed);
    return true;
}

LoRaFrame LoRaTransformer::fromCapture(const std::vector<uint8_t>& payload,
                                            const LoRaRadioProfile& profile,
                                            float rssi, float snr) const {
    LoRaFrame frame;
    frame.payload = payload;
    frame.profile = profile;
    frame.rssi = rssi;
    frame.snr = snr;
    return frame;
}
