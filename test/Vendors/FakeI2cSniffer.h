#pragma once

#include <cstdint>

inline uint32_t fakeI2cSnifferBeginCalls = 0;
inline uint32_t fakeI2cSnifferReleaseCalls = 0;
inline uint8_t fakeI2cSnifferScl = 0;
inline uint8_t fakeI2cSnifferSda = 0;

extern "C" {
inline void i2c_sniffer_begin(uint8_t scl, uint8_t sda) {
    ++fakeI2cSnifferBeginCalls;
    fakeI2cSnifferScl = scl;
    fakeI2cSnifferSda = sda;
}
inline bool i2c_sniffer_setup() { return true; }
inline void i2c_sniffer_stop() {}
inline void i2c_sniffer_release() { ++fakeI2cSnifferReleaseCalls; }
inline bool i2c_sniffer_available() { return false; }
inline char i2c_sniffer_read() { return '\0'; }
inline void i2c_sniffer_reset_buffer() {}
}

inline void resetFakeI2cSniffer() {
    fakeI2cSnifferBeginCalls = 0;
    fakeI2cSnifferReleaseCalls = 0;
    fakeI2cSnifferScl = 0;
    fakeI2cSnifferSda = 0;
}
