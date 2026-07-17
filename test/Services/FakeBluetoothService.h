#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "Interfaces/IBluetoothService.h"

class FakeBluetoothService final : public IBluetoothService {
public:
    BluetoothMode mode = BluetoothMode::NONE;
    bool connected = false;
    bool spoofResult = true;
    std::string macAddress = "AA:BB:CC:DD:EE:FF";
    std::vector<std::string> scanResults;
    std::vector<std::string> connectResults;
    std::vector<std::string> sniffLogs;

    std::vector<std::string> startedServers;
    std::vector<std::string> keyboardTexts;
    std::vector<std::string> connectAddresses;
    std::vector<std::string> spoofedMacs;
    std::vector<BluetoothMode> modeSwitches;
    std::vector<std::pair<int16_t, int16_t>> mouseMoves;
    std::vector<uint8_t> mouseReportButtons;
    uint32_t stopServerCalls = 0;
    uint32_t releaseBtClassicCalls = 0;
    uint32_t initCalls = 0;
    uint32_t deinitCalls = 0;
    uint32_t pairCalls = 0;
    uint32_t clickCalls = 0;
    uint32_t emptyReportCalls = 0;
    uint32_t startSniffCalls = 0;
    uint32_t stopSniffCalls = 0;
    int lastScanSeconds = 0;

    void startServer(const std::string& deviceName = "Bit-Pirate-Bluetooth") override {
        startedServers.push_back(deviceName);
        mode = BluetoothMode::SERVER;
    }

    void stopServer() override { ++stopServerCalls; }
    void releaseBtClassic() override { ++releaseBtClassicCalls; }

    void init(const std::string& = "Bit-Pirate-Bluetooth") override {
        ++initCalls;
    }

    void deinit() override {
        ++deinitCalls;
        mode = BluetoothMode::NONE;
        connected = false;
    }

    void pairWithAddress(const std::string&) override { ++pairCalls; }

    bool isConnected() const override { return connected; }

    void sendKeyboardText(const std::string& text) override {
        keyboardTexts.push_back(text);
    }

    void sendKeyboardReport(uint8_t, const std::array<uint8_t, 6>&) override {}

    void mouseMove(int16_t x, int16_t y) override {
        mouseMoves.push_back({x, y});
    }

    void clickMouse() override { ++clickCalls; }

    void sendMouseReport(int16_t, int16_t, uint8_t buttons) override {
        mouseReportButtons.push_back(buttons);
    }

    void sendEmptyReports() override { ++emptyReportCalls; }

    bool spoofMacAddress(const std::string& macStr) override {
        spoofedMacs.push_back(macStr);
        if (spoofResult) macAddress = macStr;
        return spoofResult;
    }

    std::string getMacAddress() override { return macAddress; }
    BluetoothMode getMode() override { return mode; }

    void switchToMode(BluetoothMode newMode) override {
        modeSwitches.push_back(newMode);
        mode = newMode;
    }

    std::vector<std::string> scanDevices(int seconds = 10) override {
        lastScanSeconds = seconds;
        return scanResults;
    }

    std::vector<std::string> connectTo(const std::string& addr) override {
        connectAddresses.push_back(addr);
        connected = !connectResults.empty();
        return connectResults;
    }

    void startPassiveSniffing() override { ++startSniffCalls; }
    void stopPassiveSniffing() override { ++stopSniffCalls; }

    std::vector<std::string> getPassiveSniffLog() override {
        auto logs = sniffLogs;
        sniffLogs.clear();
        return logs;
    }
};
