#include "TwoWireService.h"
#include <Arduino.h>

void TwoWireService::configure(uint8_t clk, uint8_t io, uint8_t rst) {
    clkPin = clk;
    ioPin = io;
    rstPin = rst;

    gpio_reset_pin((gpio_num_t)clkPin);
    gpio_reset_pin((gpio_num_t)rstPin);
    gpio_reset_pin((gpio_num_t)ioPin);

    gpio_set_direction((gpio_num_t)clkPin, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t)rstPin, GPIO_MODE_OUTPUT);
    gpio_set_direction((gpio_num_t)ioPin, GPIO_MODE_OUTPUT);

    gpio_set_level((gpio_num_t)clkPin, 0);
    gpio_set_level((gpio_num_t)rstPin, 0);
    gpio_set_level((gpio_num_t)ioPin, 1); // release
}

void TwoWireService::end() {
    gpio_set_level((gpio_num_t)clkPin, 0);
    gpio_set_level((gpio_num_t)rstPin, 0);

    gpio_set_direction((gpio_num_t)clkPin, GPIO_MODE_INPUT);
    gpio_set_pull_mode((gpio_num_t)clkPin, GPIO_FLOATING);

    gpio_set_direction((gpio_num_t)rstPin, GPIO_MODE_INPUT);
    gpio_set_pull_mode((gpio_num_t)rstPin, GPIO_FLOATING);

    gpio_set_direction((gpio_num_t)ioPin, GPIO_MODE_INPUT);
    gpio_set_pull_mode((gpio_num_t)ioPin, GPIO_FLOATING);
}

void TwoWireService::setRST(bool level) {
    gpio_set_level((gpio_num_t)rstPin, level ? 1 : 0);
}

void TwoWireService::setCLK(bool level) {
    gpio_set_level((gpio_num_t)clkPin, level ? 1 : 0);
}

void TwoWireService::setIO(bool level) {
    if (level) {
        gpio_set_direction((gpio_num_t)ioPin, GPIO_MODE_INPUT);
        gpio_set_pull_mode((gpio_num_t)ioPin, GPIO_FLOATING);
    } else {
        gpio_set_pull_mode((gpio_num_t)ioPin, GPIO_FLOATING);
        gpio_set_direction((gpio_num_t)ioPin, GPIO_MODE_OUTPUT);
        gpio_set_level((gpio_num_t)ioPin, 0);
    }
}

bool TwoWireService::readIO() {
    gpio_set_direction((gpio_num_t)ioPin, GPIO_MODE_INPUT);
    return gpio_get_level((gpio_num_t)ioPin);
}

void TwoWireService::pulseClock() {
    setCLK(false);
    delayMicroseconds(5);
    setCLK(true);
    delayMicroseconds(5);
    setCLK(false);
}

void TwoWireService::writeBit(bool bit) {
    setIO(bit);
    pulseClock();
}

bool TwoWireService::readBit() {
    setCLK(true);
    delayMicroseconds(5);
    bool bit = readIO();
    setCLK(false);
    delayMicroseconds(5);
    return bit;
}

void TwoWireService::writeByte(uint8_t byte) {
    for (uint8_t i = 0; i < 8; i++) {
        writeBit((byte >> i) & 1);
    }
}

uint8_t TwoWireService::readByte() {
    uint8_t b = 0;
    for (uint8_t i = 0; i < 8; i++) {
        bool bit = readBit();
        if (bit) b |= (1 << i);
    }
    return b;
}

void TwoWireService::sendStart() {
    setIO(true);
    setCLK(true);
    delayMicroseconds(5);
    setIO(false);
    delayMicroseconds(5);
    setCLK(false);
}

void TwoWireService::sendStop() {
    setIO(false);
    setCLK(true);
    delayMicroseconds(5);
    setIO(true);
    delayMicroseconds(5);
    setCLK(false);
}

void TwoWireService::sendCommand(uint8_t a, uint8_t b, uint8_t c) {
    sendStart();
    writeByte(a);
    writeByte(b);
    writeByte(c);
    sendStop();
}

std::vector<uint8_t> TwoWireService::readResponse(uint16_t len) {
    std::vector<uint8_t> data;
    for (uint16_t i = 0; i < len; i++) {
        data.push_back(readByte());
    }
    return data;
}

void TwoWireService::sendClocks(uint16_t ticks) {
    for (uint16_t i = 0; i < ticks; i++) {
        pulseClock();
    }
}

bool TwoWireService::waitIOHigh(uint32_t maxTicks) {
    for (uint32_t i = 0; i < maxTicks; ++i) {
        pulseClock();
        if (readIO()) return true;
    }
    return false; // timeout
}

std::vector<uint8_t> TwoWireService::performSmartCardAtr() {
    // dummy tick to 'load' the clock, avoiding us delay on first call
    setCLK(true);
    delayMicroseconds(5);
    setCLK(false);

    // Start ATR
    setIO(false);     // IO to output
    setRST(false);    // RST LOW
    delay(1);         // wait
    setRST(true);     // RST release
    pulseClock();     // Clock tick
    delayMicroseconds(50); // wait
    setRST(false);    // RST LOW
    setIO(true);      // IO to input

    return readResponse(4);
}

std::string TwoWireService::parseSmartCardAtr(const std::vector<uint8_t>& atr) {
    char line[64];
    std::string out;

    if (atr.size() < 4) {
        snprintf(line, sizeof(line), "ATR too short (%d bytes)\r\n", (int)atr.size());
        return line;
    }

    const sle44xx_atr_t* head = reinterpret_cast<const sle44xx_atr_t*>(atr.data());

    snprintf(line, sizeof(line), "   ATR: 0x%02X 0x%02X 0x%02X 0x%02X\r\n", atr[0], atr[1], atr[2], atr[3]);
    out += line;

    snprintf(line, sizeof(line), "   Protocol Type: %s (%d)\r\n",
             (head->protocol_type == 0b1010) ? "S" : "unknown",
             head->protocol_type);
    out += line;

    out += parseSmartCardStructureIdentifier(head->structure_identifier);

    out += "   Read Mode: ";
    out += head->read_with_defined_length ? "Defined Length\r\n" : "Read to End\r\n";

    if (head->data_units == 0b0000) {
        out += "   Data Units: Undefined\r\n";
    } else {
        int size = 1 << (head->data_units + 6);
        snprintf(line, sizeof(line), "   Data Units: %d\r\n", size);
        out += line;
    }

    int bit_len = 1 << head->data_units_bits;
    snprintf(line, sizeof(line), "   Data Unit Bit Length: %d\r\n", bit_len);
    out += line;

    return out;
}

std::string TwoWireService::parseSmartCardStructureIdentifier(uint8_t id) {
    std::string out = "   Structure Identifier: ";
    switch (id) {
        case 0b000:
            out += "Reserved for ISO/IEC Use\r\n";
            break;
        case 0b010:
            out += "Standard Memory Structure (Type 1)\r\n";
            break;
        case 0b110:
            out += "Proprietary Memory\r\n";
            break;
        default:
            out += "Application-Specific\r\n";
            break;
    }
    return out;
}

uint8_t TwoWireService::parseSmartCardRemainingAttempts(uint8_t statusByte) {
    resetSmartCard();
    uint8_t attemptsBits = statusByte & 0x07;
    int attempts = 0;
    for (int i = 0; i < 3; ++i) {
        if (attemptsBits & (1 << i)) {
            attempts++;
        }
    }
    return attempts;
}

std::vector<uint8_t> TwoWireService::dumpSmartCardFullMemory() {
    resetSmartCard();
    std::vector<uint8_t> dump;

    // Main memory 256 bytes
    sendCommand(0x30, 0x00, 0x00);
    for (int i = 0; i < 256; ++i) {
        dump.push_back(readByte());
    }

    // Security memory 4 bytes
    sendCommand(0x31, 0x00, 0x00);
    for (int i = 0; i < 4; ++i) {
        dump.push_back(readByte());
    }

    // Protection memory 4 bytes
    sendCommand(0x34, 0x00, 0x00);
    for (int i = 0; i < 4; ++i) {
        dump.push_back(readByte());
    }

    return dump; // total: 264 bytes
}

void TwoWireService::resetSmartCard() {
    sendClocks(256);
}

void TwoWireService::updateSmartCardSecurityAttempts(uint8_t pattern) {
    sendCommand(0x39, 0x00, pattern);
    resetSmartCard();
}

void TwoWireService::compareSmartCardVerificationData(uint8_t address, uint8_t value) {
    sendCommand(0x33, address, value);
    resetSmartCard();
}

void TwoWireService::writeSmartCardSecurityMemory(uint8_t address, uint8_t value) {
    sendCommand(0x39, address, value);
    resetSmartCard();
}

void TwoWireService::writeSmartCardProtectionMemory(uint8_t address, uint8_t value) {
    sendCommand(0x3C, address, value);
    resetSmartCard();
}

bool TwoWireService::writeSmartCardMainMemory(uint8_t address, uint8_t value) {
    sendCommand(0x38, address, value);
    resetSmartCard();

    // Read to verify
    auto readBack = readSmartCardMainMemory(address, 1);
    return !readBack.empty() && readBack[0] == value;
}

std::vector<uint8_t> TwoWireService::readSmartCardMainMemory(uint8_t startAddress, uint16_t length) {
    std::vector<uint8_t> buf;
    sendCommand(0x30, startAddress, 0x00);
    for (uint16_t i = 0; i < length; ++i) {
        buf.push_back(readByte());
    }
    return buf;
}

std::vector<uint8_t> TwoWireService::readSmartCardSecurityMemory() {
    sendCommand(0x31, 0x00, 0x00);
    return readResponse(4);
}

std::vector<uint8_t> TwoWireService::readSmartCardProtectionMemory() {
    sendCommand(0x34, 0x00, 0x00);
    return readResponse(4);
}

bool TwoWireService::updateSmartCardPSC(const uint8_t psc[3]) {
    writeSmartCardSecurityMemory(1, psc[0]);
    writeSmartCardSecurityMemory(2, psc[1]);
    writeSmartCardSecurityMemory(3, psc[2]);

    std::vector<uint8_t> secmem = readSmartCardSecurityMemory();
    if (secmem.size() < 4) {
        return false;
    }

    if (secmem[1] != psc[0] || secmem[2] != psc[1] || secmem[3] != psc[2]) {
        return false;
    }
    return true;
}

bool TwoWireService::getSmartCardPSC(uint8_t out_psc[3]) {
    std::vector<uint8_t> secmem = readSmartCardSecurityMemory();
    if (secmem.size() < 4) {
        return false;
    }
    out_psc[0] = secmem[1];
    out_psc[1] = secmem[2];
    out_psc[2] = secmem[3];
    return true;
}

bool TwoWireService::protectSmartCard() {
    uint32_t value = 0xFFFFFFFF;

    for (uint8_t i = 0; i < 4; ++i) {
        uint8_t byte = (value >> (8 * i)) & 0xFF;
        writeSmartCardProtectionMemory(i, byte);
    }

    // Lire back pour vérification
    auto check = readSmartCardProtectionMemory();
    if (check.size() != 4) return false;

    uint32_t readValue = 
        (check[3] << 24) |
        (check[2] << 16) |
        (check[1] << 8)  |
        (check[0]);

    return (readValue & value) == value;
}

bool TwoWireService::unlockSmartCard(const uint8_t psc[3]) {
    // Read sec mem
    std::vector<uint8_t> secmem = readSmartCardSecurityMemory();
    if (secmem.size() < 1) {
        return false;
    }

    // Verify attempts
    uint8_t sec = secmem[0];
    uint8_t pattern = 0;

    if (sec & 0b100) {
        pattern = 0b011;
    } else if (sec & 0b010) {
        pattern = 0b101;
    } else if (sec & 0b001) {
        pattern = 0b110;
    } else {
        // No attempts left
        return false;
    }

    // Update security attempts
    updateSmartCardSecurityAttempts(pattern);

    // Send the PSC
    compareSmartCardVerificationData(1, psc[0]);
    compareSmartCardVerificationData(2, psc[1]);
    compareSmartCardVerificationData(3, psc[2]);

    // Reinitialize security memory
    writeSmartCardSecurityMemory(0, 0xFF);

    // Read back to verify
    std::vector<uint8_t> secmemAfter = readSmartCardSecurityMemory();
    if (secmemAfter.size() < 1 || secmemAfter[0] != 0x07) {
        return false;
    }

    return true;
}