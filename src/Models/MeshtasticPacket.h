#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct MeshtasticPacket {
    uint32_t destination = 0;
    uint32_t source = 0;
    uint32_t id = 0;
    uint8_t hopLimit = 0;
    uint8_t hopStart = 0;
    uint8_t channel = 0;
    bool wantAck = false;
    bool viaMqtt = false;
    bool encrypted = true;
    uint32_t portNum = 0;
    std::string type = "ENCRYPTED";
    std::string text;
    std::vector<uint8_t> payload;
};
