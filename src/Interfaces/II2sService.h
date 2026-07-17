#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

class II2sService {
public:
    virtual ~II2sService() = default;

    virtual void configureOutput(uint8_t bclk, uint8_t lrck, uint8_t dout,
                                 uint32_t sampleRate, uint8_t bits,
                                 uint8_t percentLevel) = 0;
    virtual void configureInput(uint8_t bclk, uint8_t lrck, uint8_t din,
                                uint32_t sampleRate, uint8_t bits) = 0;
    virtual void playTone(uint32_t sampleRate, uint16_t freq, uint32_t durationMs) = 0;
    virtual void playToneInterruptible(uint32_t sampleRate, uint16_t freq,
                                       uint32_t durationMs,
                                       std::function<bool()> shouldStop) = 0;
    virtual void playPcm(const int16_t* data, size_t numBytes) = 0;
    virtual size_t recordSamples(int16_t* outBuffer, size_t sampleCount) = 0;
    virtual void end() = 0;
    virtual bool isInitialized() const = 0;
};
