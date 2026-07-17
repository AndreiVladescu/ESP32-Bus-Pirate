#pragma once

#include <Arduino.h>
#include <vector>
#include <string>
#include "Interfaces/ITwoWireService.h"

class TwoWireService : public ITwoWireService {
public:
    typedef struct {
        uint8_t protocol_type            : 4;
        uint8_t structure_identifier     : 4;
        uint8_t read_with_defined_length : 1;
        uint8_t data_units_bits          : 3;
        uint8_t data_units               : 4;
    } sle44xx_atr_t;

    void configure(uint8_t clkPin, uint8_t ioPin, uint8_t rstPin) override;
    void end() override;
    
    void setRST(bool level) override;
    void setCLK(bool level) override;
    void setIO(bool level) override;
    bool readIO() override;
    
    void pulseClock() override;
    void sendClocks(uint16_t ticks) override;
    bool waitIOHigh(uint32_t maxTicks) override;
    
    void writeBit(bool bit) override;
    bool readBit() override;
    void writeByte(uint8_t byte) override;
    uint8_t readByte() override;
    
    void sendStart() override;
    void sendStop() override;
    void sendCommand(uint8_t a, uint8_t b, uint8_t c) override;
    std::vector<uint8_t> readResponse(uint16_t len) override;
    
    // Smartcard
    std::vector<uint8_t> performSmartCardAtr() override;
    std::string parseSmartCardAtr(const std::vector<uint8_t>& atr) override;
    uint8_t parseSmartCardRemainingAttempts(uint8_t statusByte) override;
    std::string parseSmartCardStructureIdentifier(uint8_t id) override;
    std::vector<uint8_t> dumpSmartCardFullMemory() override;
    void resetSmartCard() override;
    void updateSmartCardSecurityAttempts(uint8_t pattern) override;
    void compareSmartCardVerificationData(uint8_t address, uint8_t value) override;
    void writeSmartCardSecurityMemory(uint8_t address, uint8_t value) override;
    void writeSmartCardProtectionMemory(uint8_t address, uint8_t value) override;
    bool writeSmartCardMainMemory(uint8_t address, uint8_t value) override;
    std::vector<uint8_t> readSmartCardSecurityMemory() override;
    std::vector<uint8_t> readSmartCardMainMemory(uint8_t startAddress, uint16_t length) override;
    std::vector<uint8_t> readSmartCardProtectionMemory() override;
    bool protectSmartCard() override;
    bool unlockSmartCard(const uint8_t psc[3]) override;
    bool updateSmartCardPSC(const uint8_t psc[3]) override;
    bool getSmartCardPSC(uint8_t out_psc[3]) override;

    // Start sniffing
    bool startSniffer() override;

    // Stop sniffing and restore pins to idle state.
    void stopSniffer() override;

    // Release sniffer buffers allocated on demand.
    void releaseSniffer() override;

    bool getNextSniffEvent(uint8_t& type, uint8_t& data) override;

    // Print all available events
    void printSniffOnce(Stream& out) override;


private:
    uint8_t clkPin;
    uint8_t ioPin;
    uint8_t rstPin;

    // Sniffer
    static void IRAM_ATTR clk_isr_thunk(void* arg);
    static void IRAM_ATTR io_isr_thunk(void* arg);
    void IRAM_ATTR onClkRisingISR();
    void IRAM_ATTR onIoChangeISR();
    inline void IRAM_ATTR pushEvent(uint8_t type, uint8_t data);
    bool ensureSnifferBufferAllocated();
    bool popEvent(uint8_t& type, uint8_t& data);

    // Sniffer state
    volatile bool sn_active = false;
    volatile uint8_t sn_bitIndex = 0;
    volatile uint8_t sn_currentByte = 0;
    volatile uint8_t sn_lastIO = 1;
    volatile bool isr_service_installed = false;

    // Ring buffer
    struct SniffEvent { uint8_t type; uint8_t data; };
    static constexpr uint16_t SNIFF_Q_SIZE = 1024;
    volatile SniffEvent* sn_q = nullptr;
    volatile uint16_t sn_qHead = 0; // written by isrs
    volatile uint16_t sn_qTail = 0; // read by task
    volatile bool sn_inFrame = false;
    volatile bool sn_startPending = false;
    volatile uint32_t sn_dbgOverflow = 0;

    // Sample on negative edge if needed
    static constexpr bool SNIFF_SAMPLE_ON_NEGEDGE = false;

    // Mutex for synchronizing access to the sniffing buffer
    portMUX_TYPE sn_mux = portMUX_INITIALIZER_UNLOCKED;
};
