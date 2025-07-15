#pragma once

#include <stdint.h>
#include <Arduino.h>
#include <functional>
#include <stddef.h>
#include <string>
#include "driver/i2s.h"

class I2sService {
public:
    void configureOutput(uint8_t bclk, uint8_t lrck, uint8_t dout, uint32_t sampleRate = 44100, uint8_t bits = 16);
    void configureInput(uint8_t bclk, uint8_t lrck, uint8_t din, uint32_t sampleRate = 44100, uint8_t bits = 16);

    void playTone(uint16_t freq, uint16_t durationMs);
    void playToneInterruptible(uint16_t freq, uint32_t durationMs, std::function<bool()> shouldStop);
    void playRaw(const int16_t* samples, size_t count);
    size_t recordSamples(int16_t* outBuffer, size_t sampleCount);
    void stop();
    bool isInitialized() const;

private:
    bool initialized = false;
    i2s_port_t port = I2S_NUM_0;
};
