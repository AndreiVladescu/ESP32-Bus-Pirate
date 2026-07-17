#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

enum class BluetoothMode {
    NONE,
    SERVER,
    CLIENT
};

class IBluetoothService {
public:
    virtual ~IBluetoothService() = default;

    virtual void startServer(const std::string& deviceName = "Bit-Pirate-Bluetooth") = 0;
    virtual void stopServer() = 0;
    virtual void releaseBtClassic() = 0;

    virtual void init(const std::string& deviceName = "Bit-Pirate-Bluetooth") = 0;
    virtual void deinit() = 0;

    virtual void pairWithAddress(const std::string& addrStr) = 0;

    virtual bool isConnected() const = 0;

    virtual void sendKeyboardText(const std::string& text) = 0;
    virtual void sendKeyboardReport(uint8_t modifier, const std::array<uint8_t, 6>& keys) = 0;

    virtual void mouseMove(int16_t x, int16_t y) = 0;
    virtual void clickMouse() = 0;
    virtual void sendMouseReport(int16_t x, int16_t y, uint8_t buttons) = 0;

    virtual void sendEmptyReports() = 0;
    virtual bool spoofMacAddress(const std::string& macStr) = 0;
    virtual std::string getMacAddress() = 0;
    virtual BluetoothMode getMode() = 0;
    virtual void switchToMode(BluetoothMode newMode) = 0;

    virtual std::vector<std::string> scanDevices(int seconds = 10) = 0;
    virtual std::vector<std::string> connectTo(const std::string& addr) = 0;

    virtual void startPassiveSniffing() = 0;
    virtual void stopPassiveSniffing() = 0;
    virtual std::vector<std::string> getPassiveSniffLog() = 0;
};
