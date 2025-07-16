#pragma once
#include <string>
#include <vector>
#include "Arduino.h"
#include <XModem.h>
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_rom_gpio.h"
#include "hal/uart_types.h"
#include "soc/uart_periph.h"
#include "Models/ByteCode.h"
#include <SD.h>

#define UART_PORT UART_NUM_1

class UartService {
public:
    void configure(unsigned long baud, uint32_t config, uint8_t rx, uint8_t tx, bool inverted);
    void print(const std::string& msg);
    void println(const std::string& msg);
    char read();
    std::string readLine();
    bool available() const;
    void write(char c);
    void write(const std::string& str);
    std::string executeByteCode(const std::vector<ByteCode>& bytecodes);
    void switchBaudrate(unsigned long newBaud);
    void flush();
    void clearUartBuffer();
    void end();
    uint32_t buildUartConfig(uint8_t dataBits, char parity, uint8_t stopBits);
    void initXmodem();
    bool xmodemReceiveToFile(File& file);
    bool xmodemSendFile(File& file);
    static void blockLookupHandler(void* blk_id, size_t idSize, byte* data, size_t dataSize);
    static bool receiveBlockHandler(void* blk_id, size_t idSize, byte* data, size_t dataSize);
    void setXmodemReceiveHandler(bool (*handler)(void*, size_t, byte*, size_t));
    void setXmodemSendHandler(void (*handler)(void*, size_t, byte*, size_t));
    void setXmodemBlockSize(int32_t size);
    void setXmodemIdSize(int8_t size);
    void setXmodemCrc(bool enabled);
    int32_t getXmodemBlockSize() const;
    int8_t getXmodemIdSize() const;

private:
    XModem xmodem;
    static File* currentFile;
    int32_t xmodemBlockSize = 128;
    int8_t xmodemIdSize = 1;
    XModem::ProtocolType xmodemProtocol = XModem::ProtocolType::CRC_XMODEM;
};
