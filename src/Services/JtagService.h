#pragma once

// Porting https://github.com/Aodrulez/blueTag/ to ESP32
// Thanks to Atul Alex Cherian the original author

/* 
    [ blueTag - Hardware hacker's multi-tool based on RP2040 dev boards ]

        Inspired by JTAGulator. 

    [References & special thanks]
        https://github.com/grandideastudio/jtagulator
        https://research.kudelskisecurity.com/2019/05/16/swd-arms-alternative-to-jtag/
        https://github.com/jbentham/picoreg
        https://github.com/szymonh/SWDscan
        Yusufss4 (https://gist.github.com/amullins83/24b5ef48657c08c4005a8fab837b7499?permalink_comment_id=4554839#gistcomment-4554839)
        Arm Debug Interface Architecture Specification (debug_interface_v5_2_architecture_specification_IHI0031F.pdf)
        
        Flashrom support : 
            https://www.flashrom.org/supported_hw/supported_prog/serprog/serprog-protocol.html
            https://github.com/stacksmashing/pico-serprog

        Openocd support  : 
            http://dangerousprototypes.com/blog/2009/10/09/bus-pirate-raw-bitbang-mode/
            http://dangerousprototypes.com/blog/2009/10/27/binary-raw-wire-mode/
            https://github.com/grandideastudio/jtagulator/blob/master/PropOCD.spin
            https://github.com/DangerousPrototypes/Bus_Pirate/blob/master/Firmware/binIO.c

        USB-to-Serial support :
            https://github.com/xxxajk/pico-uart-bridge
            https://github.com/Noltari/pico-uart-bridge

        CMSIS-DAP support :
            https://github.com/majbthrd/DapperMime
            https://github.com/raspberrypi/debugprobe
*/

#include <Arduino.h>
#include <cstdint>
#include <vector>
#include "Interfaces/IJtagService.h"

class JtagService : public IJtagService {
public:
    void configureJtag(uint8_t tck, uint8_t tms, uint8_t tdi, uint8_t tdo, int trst = -1) override;
    
    bool scanJtagDevice(
        const std::vector<uint8_t>& pins,
        uint8_t& outTDI, uint8_t& outTDO,
        uint8_t& outTCK, uint8_t& outTMS,
        int& outTRST,
        std::vector<uint32_t>& outDeviceIDs,
        bool pulsePins,
        void (*onProgress)(size_t, size_t)
    ) override;

    bool scanSwdDevice(const std::vector<uint8_t>& pins, uint8_t& foundIO, uint8_t& foundCLK, uint32_t& idcodeOut) override;

private:
    // JTAG pins
    int _pinTCK = -1;
    int _pinTMS = -1;
    int _pinTDI = -1;
    int _pinTDO = -1;
    int _pinTRST = -1;

    // SWD pins
    int _pinSWDIO = -1;
    int _pinSWCLK = -1;

    // JTAG helpers
    void tckPulse();
    void tdiWrite(bool val);
    void tmsWrite(bool val);
    bool tdoRead();
    void restoreIdle();
    void enterShiftDR();
    void enterShiftIR();
    uint32_t bitReverse(uint32_t n);
    uint32_t shiftArray(uint32_t pattern, int bits);
    uint32_t sendData(uint32_t pattern, int bits);
    int detectDevices();
    void getDeviceIDs(int count, std::vector<uint32_t>& ids);
    uint32_t bypassTest(int count, uint32_t pattern);
    bool isValidDeviceID(uint32_t id);
    void jtagResetChannels(const std::vector<uint8_t>& pins, int trstPin, bool pulsePins);
    void jtagInitChannels(const std::vector<uint8_t>& pins, bool pulsePins);

    // SWD helpers
    void swdClockPulse();
    void swdSetReadMode();
    void swdSetWriteMode();
    bool swdReadBit();
    void swdWriteBit(bool bit);
    void swdWriteBits(uint32_t value, int bitLength);
    bool swdReadAck();
    void swdResetLineSWDJ();
    void swdToJTAG();
    void swdArmWakeUp();
    bool swdTrySWDJ(uint32_t& idcodeOut);
    void swdDelay() ;
};
