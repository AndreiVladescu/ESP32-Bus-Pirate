#pragma once

#include <Selectors/HorizontalSelector.h>
#include <Enums/TerminalTypeEnum.h>

class TerminalTypeConfigurator {
public:
    TerminalTypeConfigurator(HorizontalSelector& selector);
    TerminalTypeEnum configure();

private:
    HorizontalSelector& selector;
};
