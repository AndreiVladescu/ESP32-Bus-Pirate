#include "Controllers/ExpanderController.h"

/*
Constructor
*/
ExpanderController::ExpanderController(ITerminalView& terminalView,
                           IInput& terminalInput,
                           UartService& uartService,
                           ArgTransformer& argTransformer,
                           UserInputManager& userInputManager,
                           HelpShell& helpShell)
    : terminalView(terminalView),
      terminalInput(terminalInput),
      uartService(uartService),
      argTransformer(argTransformer),
      userInputManager(userInputManager),
      helpShell(helpShell) {
}

/*
Entry point for Expander command
*/
void ExpanderController::handleCommand(const TerminalCommand& cmd) {
    if (!configured) {
        handleConfig();
    } else {
        handleBridge();
    }
}

/*
Ensure configured
*/
void ExpanderController::ensureConfigured() {
    if (!configured) {
        handleConfig();
    } else {
        uartService.write("\n");
        handleBridge();
    }

}

/*
Bridge
*/
void ExpanderController::handleBridge() {
    terminalView.println("Expander Connected: Starting... Type 'exit' to stop.");

    std::string txLine;

    while (true) {
        while (uartService.available()) {
            char c = uartService.read();
            terminalView.print(std::string(1, c));
        }

        // ESC seq
        char c = terminalInput.readChar();
        if (c != KEY_NONE) {
            if (c == '\x1B') {
                uartService.write(c);

                unsigned long start = millis();
                while (millis() - start < 20) {
                    char c2 = terminalInput.readChar();
                    if (c2 != KEY_NONE) {
                        uartService.write(c2);

                        start = millis();
                        while (millis() - start < 20) {
                            char c3 = terminalInput.readChar();
                            if (c3 != KEY_NONE) {
                                uartService.write(c3);
                                break;
                            }
                        }
                        break;
                    }
                }
                continue;
            }

            uartService.write(c);

            if (c == '\r' || c == '\n') {
                if (txLine == "exit") {
                    terminalView.println("\n\n\rExpander session closed.");
                    terminalView.println("Returning to ESP32 Bus Pirate...\n");
                    uartService.flush();
                    configured = false;
                    return;
                }
                txLine.clear();
            } else if (c == '\b' || c == 127) {
                if (!txLine.empty()) {
                    txLine.pop_back();
                }
            } else {
                txLine += c;
            }
        }

        delay(1);
    }
}

/*
Config
*/
void ExpanderController::handleConfig() {
    terminalView.println("Expander UART Configuration:");

    const auto& forbidden = state.getProtectedPins();

    uint8_t rxPin = userInputManager.readValidatedPinNumber(
        "RX GPIO number",
        state.getUartRxPin(),
        forbidden
    );
    state.setUartRxPin(rxPin);

    uint8_t txPin = userInputManager.readValidatedPinNumber(
        "TX GPIO number",
        state.getUartTxPin(),
        forbidden
    );
    state.setUartTxPin(txPin);

    uint32_t config = uartService.buildUartConfig(dataBits, parityChar, stopBits);
    uartService.configure(baud, config, rxPin, txPin, inverted);

    terminalView.println("Expander UART configured (115200 8N1).");
    terminalView.println("Sending handshake...");

    // Flush RX
    while (uartService.available()) {
        uartService.read();
    }

    delay(100);

    // Send a few ENTER to bring the slave back to its main prompt
    for (int i = 0; i < 8; ++i) {
        uartService.write('\r');
        uartService.write('\n');
        delay(20);
    }

    // flush what came back after the reset
    while (uartService.available()) {
        uartService.read();
    }

    // Handshake to detect if the C5 is here
    uartService.write("handshake\n");

    std::string rxBuffer;
    unsigned long start = millis();
    bool handshakeOk = false;

    while (millis() - start < 2000) {
        while (uartService.available()) {
            char c = uartService.read();
            rxBuffer += c;

            if (rxBuffer.size() > 128) {
                rxBuffer.erase(0, rxBuffer.size() - 128);
            }

            if (rxBuffer.find("[[BP-HANDSHAKE-OK]]") != std::string::npos) {
                handshakeOk = true;
                break;
            }
        }

        if (handshakeOk) {
            break;
        }

        delay(5);
    }

    if (!handshakeOk) {
        terminalView.println("Expander handshake failed.");
        terminalView.println("Try to swap RX/TX GPIOs.");
        terminalView.println("Ensure the Expander is powered.\n");
        configured = false;
        state.setCurrentMode(ModeEnum::HIZ);
        return;
    }

    terminalView.println("Expander handshake OK.");
    terminalView.println("");
    terminalView.println(" [ℹ️  INFORMATION] ");
    terminalView.println(" You are now connected to the Expander.");
    terminalView.println(" All commands are sent directly to it.\n");

    configured = true;
    handleBridge();
}
