#pragma once

#include <cstdint>

class IInput;
class ITerminalView;
class IUtilityService;

class IUartSnifferService {
public:
    virtual ~IUartSnifferService() = default;

    virtual void sniffText(ITerminalView& terminalView,
                           IInput& terminalInput,
                           IUtilityService& utilityService,
                           unsigned long baud,
                           uint32_t config,
                           bool inverted,
                           uint8_t rxPin1,
                           uint8_t rxPin2) = 0;

    virtual void sniffRaw(ITerminalView& terminalView,
                          IInput& terminalInput,
                          IUtilityService& utilityService,
                          unsigned long baud,
                          uint32_t config,
                          bool inverted,
                          uint8_t rxPin1,
                          uint8_t rxPin2) = 0;
};
