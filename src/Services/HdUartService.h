#pragma once

#include <vector>
#include <Arduino.h>
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_rom_gpio.h"
#include "hal/uart_types.h"
#include "soc/uart_periph.h"
#include "Models/ByteCode.h"
#include "Interfaces/IHdUartService.h"

#define HD_UART_PORT UART_NUM_2
#define UART_RX_BUFFER_SIZE 256

class HdUartService : public IHdUartService {
public:
    void configure(unsigned long baud, uint8_t dataBits, char parity, uint8_t stopBits, uint8_t ioPin, bool inverted) override;
    void write(uint8_t data) override;
    void write(const std::string& str) override;
    bool available() const override;
    char read() override;
    std::string readLine() override;
    std::string executeByteCode(const std::vector<ByteCode>& bytecodes) override;
    void flush() override;
    uart_config_t buildUartConfig(unsigned long baud, uint8_t bits, char parity, uint8_t stop);
    void end() override;

private:
    uint8_t ioPin;
    unsigned long baudRate;
    uint32_t serialConfig;
    bool isInverted;

};
