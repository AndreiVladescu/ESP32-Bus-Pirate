#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "Models/MeshtasticPacket.h"
#include "Models/LoRaRadioProfile.h"

class MeshtasticService {
public:
    struct Preset { const char* name; uint16_t bandwidth; uint8_t sf; uint8_t cr; };

    static const Preset* presets(size_t& count);
    static bool profileFor(const std::string& name, float frequency, LoRaRadioProfile& profile);
    bool setKey(const std::string& value);
    void clearKey();
    bool hasKey() const { return !key_.empty(); }
    size_t keyBits() const { return key_.size() * 8; }
    bool inspect(const std::vector<uint8_t>& frame, MeshtasticPacket& packet,
                 std::string& error) const;
    bool buildTextFrame(const std::string& text,
                        const std::string& channelName,
                        uint32_t source,
                        uint32_t packetId,
                        std::vector<uint8_t>& frame,
                        std::string& error) const;
    uint8_t channelHash(const std::string& channelName) const;

private:
    static bool readVarint(const uint8_t*& p, const uint8_t* end, uint64_t& value);
    static bool skipField(uint8_t wire, const uint8_t*& p, const uint8_t* end);
    static std::string portName(uint32_t port);
    static void appendLe32(std::vector<uint8_t>& bytes, uint32_t value);
    static void appendVarint(std::vector<uint8_t>& bytes, uint64_t value);
    bool decodeData(const std::vector<uint8_t>& bytes, MeshtasticPacket& packet) const;
    bool cryptPayload(uint32_t source, uint32_t packetId,
                      const std::vector<uint8_t>& input,
                      std::vector<uint8_t>& output) const;
    bool decryptPayload(const MeshtasticPacket& packet,
                        std::vector<uint8_t>& output) const;
    std::vector<uint8_t> key_;
};
