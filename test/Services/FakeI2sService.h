#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

#include "Interfaces/II2sService.h"

class FakeI2sService final : public II2sService {
public:
    struct Configuration {
        uint8_t bclk = 0;
        uint8_t lrck = 0;
        uint8_t data = 0;
        uint32_t sampleRate = 0;
        uint8_t bits = 0;
        uint8_t level = 0;
    };

    struct Tone {
        uint32_t sampleRate = 0;
        uint16_t frequency = 0;
        uint32_t durationMs = 0;
    };

    Configuration lastOutput;
    Configuration lastInput;
    std::vector<Tone> tones;
    std::vector<int16_t> samples;
    Tone lastInterruptibleTone;
    uint32_t outputConfigureCalls = 0;
    uint32_t inputConfigureCalls = 0;
    uint32_t interruptibleToneCalls = 0;
    uint32_t pcmCalls = 0;
    uint32_t recordCalls = 0;
    uint32_t endCalls = 0;
    size_t lastPcmBytes = 0;
    bool initialized = true;

    void configureOutput(uint8_t bclk, uint8_t lrck, uint8_t dout,
                         uint32_t sampleRate, uint8_t bits,
                         uint8_t percentLevel) override {
        ++outputConfigureCalls;
        lastOutput = {bclk, lrck, dout, sampleRate, bits, percentLevel};
    }

    void configureInput(uint8_t bclk, uint8_t lrck, uint8_t din,
                        uint32_t sampleRate, uint8_t bits) override {
        ++inputConfigureCalls;
        lastInput = {bclk, lrck, din, sampleRate, bits, 0};
    }

    void playTone(uint32_t sampleRate, uint16_t freq, uint32_t durationMs) override {
        tones.push_back({sampleRate, freq, durationMs});
    }

    void playToneInterruptible(uint32_t sampleRate, uint16_t freq,
                               uint32_t durationMs,
                               std::function<bool()>) override {
        ++interruptibleToneCalls;
        lastInterruptibleTone = {sampleRate, freq, durationMs};
    }

    void playPcm(const int16_t*, size_t numBytes) override {
        ++pcmCalls;
        lastPcmBytes = numBytes;
    }

    size_t recordSamples(int16_t* outBuffer, size_t sampleCount) override {
        ++recordCalls;
        const size_t count = std::min(sampleCount, samples.size());
        std::copy_n(samples.begin(), count, outBuffer);
        return count;
    }

    void end() override { ++endCalls; }
    bool isInitialized() const override { return initialized; }
};
