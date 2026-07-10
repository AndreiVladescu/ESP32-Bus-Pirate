#pragma once

#include <string>
#include <vector>
#include "Interfaces/IDeviceView.h"
#include "Interfaces/IInput.h"
#include "Data/InputKeys.h"
#include "Enums/TerminalTypeEnum.h"
#include <Arduino.h>

class HorizontalSelector {
public:
    HorizontalSelector(IDeviceView& display, IInput& input);

    int select(
        const std::string& title,
        const std::vector<std::string>& options,
        const std::string& description1 = "",
        const std::string& description2 = ""
    );

    int selectHeadless();

private:
    IDeviceView& display;
    IInput& input;
};
