#pragma once
#include "IInput.h"
#include "Servers/WebSocketServer.h"

class WebTerminalInput : public IInput {
public:
    WebTerminalInput(WebSocketServer& server);

    char handler() override;
    char readChar() override;
    void waitPress() override;

private:
    WebSocketServer& server;
};
