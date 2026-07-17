#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

class IJtagService {
public:
    virtual ~IJtagService() = default;

    virtual void configureJtag(uint8_t tck, uint8_t tms, uint8_t tdi,
                               uint8_t tdo, int trst = -1) = 0;
    virtual bool scanJtagDevice(
        const std::vector<uint8_t>& pins,
        uint8_t& outTDI, uint8_t& outTDO,
        uint8_t& outTCK, uint8_t& outTMS,
        int& outTRST,
        std::vector<uint32_t>& outDeviceIDs,
        bool pulsePins,
        void (*onProgress)(size_t, size_t)) = 0;
    virtual bool scanSwdDevice(const std::vector<uint8_t>& pins,
                               uint8_t& foundIO, uint8_t& foundCLK,
                               uint32_t& idcodeOut) = 0;
};
