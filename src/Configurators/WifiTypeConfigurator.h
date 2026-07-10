#pragma once

#include <Enums/TerminalTypeEnum.h>
#include <Boards/Cardputer/CardWifiSetup.h>
#include <Boards/StickS3/StickWifiSetup.h>
#include <Boards/Common/Wifi/DefaultWifiSetup.h>
#include <Boards/TEmbed/TembedWifiSetup.h>
#include <Boards/TDisplayS3/TdisplayWifiSetup.h>
#include <Boards/WaveshareS3Geek/WaveshareS3GeekWifiSetup.h>
#include <Boards/VisionMasterT190/VisionMasterT190WifiSetup.h>
#include <Interfaces/IDeviceView.h>
#include <Interfaces/IInput.h>
#include <string>
#include <WiFi.h>

class WifiTypeConfigurator {
public:
    WifiTypeConfigurator(IDeviceView& view, IInput& input)
        : view(view), input(input) {}

    std::string configure(TerminalTypeEnum& terminalType);

private:
    IDeviceView& view;
    IInput& input;
};
