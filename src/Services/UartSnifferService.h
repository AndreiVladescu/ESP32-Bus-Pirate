#pragma once

#include "Interfaces/IUartSnifferService.h"

class HardwareSerial;
class IUartPort;

class UartSnifferService final : public IUartSnifferService {
public:
    UartSnifferService(IUartPort& firstPort,
                       HardwareSerial* firstSerial,
                       IUartPort& secondPort,
                       HardwareSerial* secondSerial);

    void sniffText(ITerminalView& terminalView,
                   IInput& terminalInput,
                   IUtilityService& utilityService,
                   unsigned long baud,
                   uint32_t config,
                   bool inverted,
                   uint8_t rxPin1,
                   uint8_t rxPin2) override;

    void sniffRaw(ITerminalView& terminalView,
                  IInput& terminalInput,
                  IUtilityService& utilityService,
                  unsigned long baud,
                  uint32_t config,
                  bool inverted,
                  uint8_t rxPin1,
                  uint8_t rxPin2) override;

private:
    IUartPort& firstPort;
    HardwareSerial* firstSerial;
    IUartPort& secondPort;
    HardwareSerial* secondSerial;

    void configurePorts(unsigned long baud,
                        uint32_t config,
                        bool inverted,
                        uint8_t rxPin1,
                        uint8_t rxPin2);
};
