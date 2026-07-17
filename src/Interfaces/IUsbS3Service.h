#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

class IUsbS3Service {
public:
    virtual ~IUsbS3Service() = default;

    virtual bool isKeyboardActive() const = 0;
    virtual bool isStorageActive() const = 0;
    virtual bool isMouseActive() const = 0;
    virtual bool isGamepadActive() const = 0;
    virtual bool isHostActive() const = 0;
    virtual bool isSystemControlActive() const = 0;

    virtual void keyboardBegin() = 0;
    virtual void keyboardSendString(const std::string& text) = 0;
    virtual void keyboardSendChunkedString(const std::string& data,
                                           size_t chunkSize,
                                           unsigned long delayBetweenChunks) = 0;

    virtual void storageBegin(uint8_t cs, uint8_t clk, uint8_t miso, uint8_t mosi) = 0;

    virtual void mouseBegin() = 0;
    virtual void mouseMove(int x, int y) = 0;
    virtual void mouseClick(int button) = 0;
    virtual void mouseRelease(int button) = 0;

    virtual void gamepadBegin() = 0;
    virtual void gamepadPress(const std::string& name) = 0;

    virtual bool usbHostBegin() = 0;
    virtual std::string usbHostTick() = 0;
    virtual void usbHostEnd() = 0;

    virtual void systemControlBegin() = 0;
    virtual void systemControlEnd() = 0;
    virtual void systemSleep() = 0;
    virtual void systemWake() = 0;
    virtual void systemPowerOff(uint32_t holdMs = 10) = 0;

    virtual void configure(const char* productStr,
                           const char* manufacturerStr,
                           const char* serialStr,
                           uint16_t vid,
                           uint16_t pid,
                           const char* webUSBString) = 0;
    virtual void reset() = 0;
    virtual std::string getUsbSerialFromEfuseMac() = 0;
};
