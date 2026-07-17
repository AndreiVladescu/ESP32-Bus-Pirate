#pragma once
#include <string>
#include "Models/TerminalCommand.h"
#if __has_include("Vendors/ArduinoJson.h")
#include "Vendors/ArduinoJson.h"
#else
#include <ArduinoJson.h>
#endif

class WebRequestTransformer {
public:
    TerminalCommand toTerminalCommand(const std::string& body);
    std::string toTerminalCommandRaw(const std::string& body);
    std::string toJsonResponse(const std::string& cliOutput);
};
