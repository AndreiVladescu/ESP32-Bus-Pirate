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
#include "Interfaces/IUartService.h"
#include <SD.h>
#include <map>

#define UART_PORT UART_NUM_1

class UartService : public IUartService {
public:
    using PinActivity = UartPinActivity;

    void configure(unsigned long baud, uint32_t config, uint8_t rx, uint8_t tx, bool inverted, HardwareSerial* serial = nullptr, bool noAllocation = false) override;
    void release() override;
    void print(const std::string& msg) override;
    void println(const std::string& msg) override;
    char read() override;
    std::string readLine() override;
    bool available() const override;
    void write(char c) override;
    void write(const char* str) override;
    void write(const std::string& str) override;
    void setRxFIFOFull(uint8_t fifoBytes) override;
    void setDefaultRxFIFOFull() override;
    std::string executeByteCode(const std::vector<ByteCode>& bytecodes) override;
    void switchBaudrate(unsigned long newBaud) override;
    void flush() override;
    void clearUartBuffer() override;
    void end() override;
    bool isInstalled() const override;
    uint32_t buildUartConfig(uint8_t dataBits, char parity, uint8_t stopBits) override;
    void initXmodem() override;
    bool xmodemReceiveToFile(File& file) override;
    bool xmodemSendFile(File& file) override;
    static void blockLookupHandler(void* blk_id, size_t idSize, uint8_t* data, size_t dataSize);
    static bool receiveBlockHandler(void* blk_id, size_t idSize, uint8_t* data, size_t dataSize);
    void setXmodemReceiveHandler(bool (*handler)(void*, size_t, uint8_t*, size_t)) override;
    void setXmodemSendHandler(void (*handler)(void*, size_t, uint8_t*, size_t)) override;
    void setXmodemBlockSize(int32_t size) override;
    void setXmodemIdSize(int8_t size) override;
    void setXmodemCrc(bool enabled) override;
    int32_t getXmodemBlockSize() const override;
    int8_t getXmodemIdSize() const override;
    PinActivity measureUartActivity(uint8_t pin, uint32_t windowMs = 100, bool pullup = true) override;
    std::vector<PinActivity> scanUartActivity(const std::vector<uint8_t>& pins,
                                              uint32_t windowMs = 100,
                                              uint32_t minEdges = 10,
                                              bool pullup = true) override;
    uint32_t detectBaudByEdge(uint8_t pin, uint32_t totalMs = 5000, uint32_t windowMs = 300, uint32_t minEdges = 30, bool pullup = true) override;
    std::vector<uint32_t> getBaudList() const override {
        return std::vector<uint32_t>(kBaudRates, kBaudRates + kBaudRatesCount);
    }
private:
    XModem xmodem;
    static File* currentFile;
    int32_t xmodemBlockSize = 128;
    int8_t xmodemIdSize = 1;
    XModem::ProtocolType xmodemProtocol = XModem::ProtocolType::CRC_XMODEM;

    static void IRAM_ATTR onGpioEdge(void* arg);
    inline static volatile uint32_t* edgeCounts = nullptr;
    inline static volatile uint32_t* edgeIntervals = nullptr;
    inline static constexpr uint8_t kMaxEdgeIntervals = 64;
    inline static volatile uint32_t lastEdgeTimeUs = 0;
    inline static volatile uint8_t intervalCount = 0;
    inline static volatile bool isrInstalled = false;
    inline static bool buffersAllocated = false;
    inline static constexpr uint32_t kBaudRates[] = {
        // Legacy 
        110, 300, 600, 1200, 1800, 2000, 2400, 3600, 4800, 7200,
        9600, 10400, 14400, 16000, 19200,

        // Mid-range
        28800, 31250, 32000, 33600, 38400,
        56000, 57600, 64000, 76800,

        // High 
        100000, 115200, 125000, 128000, 153600,
        200000, 230400, 250000, 256000, 307200,

        // Very high
        460800, 500000, 576000, 614400, 750000,
        921600, 1000000, 1152000, 1228800, 1500000,

        // Extreme
        2000000, 2500000, 3000000, 3500000, 4000000
    };

    static constexpr size_t kBaudRatesCount = sizeof(kBaudRates) / sizeof(kBaudRates[0]);
    HardwareSerial* _serial;
    bool installed = false;
};
