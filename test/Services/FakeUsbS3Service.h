#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "Interfaces/IUsbS3Service.h"

class FakeUsbS3Service final : public IUsbS3Service {
public:
    struct ConfigureCall {
        std::string product;
        std::string manufacturer;
        std::string serial;
        uint16_t vid = 0;
        uint16_t pid = 0;
        std::string webUsb;
    };

    struct StorageCall {
        uint8_t cs = 0;
        uint8_t clk = 0;
        uint8_t miso = 0;
        uint8_t mosi = 0;
    };

    struct MouseMoveCall {
        int x = 0;
        int y = 0;
    };

    bool keyboardActive = false;
    bool storageActive = false;
    bool mouseActive = false;
    bool gamepadActive = false;
    bool hostActive = false;
    bool systemControlActive = false;
    bool usbHostBeginResult = true;
    std::string hostTickResult;
    std::string efuseSerial = "EFUSE-SERIAL";

    std::vector<ConfigureCall> configureCalls;
    std::vector<StorageCall> storageCalls;
    std::vector<std::string> keyboardStrings;
    std::vector<std::string> keyboardChunkedStrings;
    std::vector<size_t> keyboardChunkSizes;
    std::vector<unsigned long> keyboardChunkDelays;
    std::vector<MouseMoveCall> mouseMoves;
    std::vector<int> mouseClicks;
    std::vector<int> mouseReleases;
    std::vector<std::string> gamepadPresses;
    uint32_t keyboardBeginCalls = 0;
    uint32_t storageBeginCalls = 0;
    uint32_t mouseBeginCalls = 0;
    uint32_t gamepadBeginCalls = 0;
    uint32_t usbHostBeginCalls = 0;
    uint32_t usbHostTickCalls = 0;
    uint32_t usbHostEndCalls = 0;
    uint32_t systemControlBeginCalls = 0;
    uint32_t systemControlEndCalls = 0;
    uint32_t systemSleepCalls = 0;
    uint32_t systemWakeCalls = 0;
    uint32_t systemPowerOffCalls = 0;
    uint32_t lastPowerOffHoldMs = 0;
    uint32_t resetCalls = 0;

    bool isKeyboardActive() const override { return keyboardActive; }
    bool isStorageActive() const override { return storageActive; }
    bool isMouseActive() const override { return mouseActive; }
    bool isGamepadActive() const override { return gamepadActive; }
    bool isHostActive() const override { return hostActive; }
    bool isSystemControlActive() const override { return systemControlActive; }

    void keyboardBegin() override {
        ++keyboardBeginCalls;
        keyboardActive = true;
    }

    void keyboardSendString(const std::string& text) override {
        keyboardStrings.push_back(text);
    }

    void keyboardSendChunkedString(const std::string& data,
                                   size_t chunkSize,
                                   unsigned long delayBetweenChunks) override {
        keyboardChunkedStrings.push_back(data);
        keyboardChunkSizes.push_back(chunkSize);
        keyboardChunkDelays.push_back(delayBetweenChunks);
    }

    void storageBegin(uint8_t cs, uint8_t clk, uint8_t miso, uint8_t mosi) override {
        ++storageBeginCalls;
        storageCalls.push_back({cs, clk, miso, mosi});
    }

    void mouseBegin() override {
        ++mouseBeginCalls;
        mouseActive = true;
    }

    void mouseMove(int x, int y) override { mouseMoves.push_back({x, y}); }
    void mouseClick(int button) override { mouseClicks.push_back(button); }
    void mouseRelease(int button) override { mouseReleases.push_back(button); }

    void gamepadBegin() override {
        ++gamepadBeginCalls;
        gamepadActive = true;
    }

    void gamepadPress(const std::string& name) override { gamepadPresses.push_back(name); }

    bool usbHostBegin() override {
        ++usbHostBeginCalls;
        hostActive = usbHostBeginResult;
        return usbHostBeginResult;
    }

    std::string usbHostTick() override {
        ++usbHostTickCalls;
        return hostTickResult;
    }

    void usbHostEnd() override {
        ++usbHostEndCalls;
        hostActive = false;
    }

    void systemControlBegin() override {
        ++systemControlBeginCalls;
        systemControlActive = true;
    }

    void systemControlEnd() override {
        ++systemControlEndCalls;
        systemControlActive = false;
    }

    void systemSleep() override { ++systemSleepCalls; }
    void systemWake() override { ++systemWakeCalls; }

    void systemPowerOff(uint32_t holdMs = 10) override {
        ++systemPowerOffCalls;
        lastPowerOffHoldMs = holdMs;
    }

    void configure(const char* productStr,
                   const char* manufacturerStr,
                   const char* serialStr,
                   uint16_t vid,
                   uint16_t pid,
                   const char* webUSBString) override {
        configureCalls.push_back({
            productStr == nullptr ? "" : productStr,
            manufacturerStr == nullptr ? "" : manufacturerStr,
            serialStr == nullptr ? "" : serialStr,
            vid,
            pid,
            webUSBString == nullptr ? "" : webUSBString
        });
    }

    void reset() override { ++resetCalls; }
    std::string getUsbSerialFromEfuseMac() override { return efuseSerial; }
};
