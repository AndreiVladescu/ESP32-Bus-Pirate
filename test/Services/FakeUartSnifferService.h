#pragma once

#include <cstdint>

#include "Interfaces/IUartSnifferService.h"

class FakeUartSnifferService final : public IUartSnifferService {
public:
    struct Call {
        unsigned long baud = 0;
        uint32_t config = 0;
        bool inverted = false;
        uint8_t rxPin1 = 0;
        uint8_t rxPin2 = 0;
    };

    uint32_t textCalls = 0;
    uint32_t rawCalls = 0;
    Call lastText;
    Call lastRaw;

    void sniffText(ITerminalView&,
                   IInput&,
                   IUtilityService&,
                   unsigned long baud,
                   uint32_t config,
                   bool inverted,
                   uint8_t rxPin1,
                   uint8_t rxPin2) override {
        ++textCalls;
        lastText = {baud, config, inverted, rxPin1, rxPin2};
    }

    void sniffRaw(ITerminalView&,
                  IInput&,
                  IUtilityService&,
                  unsigned long baud,
                  uint32_t config,
                  bool inverted,
                  uint8_t rxPin1,
                  uint8_t rxPin2) override {
        ++rawCalls;
        lastRaw = {baud, config, inverted, rxPin1, rxPin2};
    }
};
