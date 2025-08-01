#include "Controllers/DioController.h"
#include <sstream>
#include <algorithm>

/*
Constructor
*/
DioController::DioController(ITerminalView& terminalView, IInput& terminalInput, PinService& pinService, ArgTransformer& argTransformer)
    : terminalView(terminalView), terminalInput(terminalInput), pinService(pinService), argTransformer(argTransformer) {}

/*
Entry point to handle a DIO command
*/
void DioController::handleCommand(const TerminalCommand& cmd) {
    if (cmd.getRoot() == "sniff") handleSniff(cmd);
    else if (cmd.getRoot() == "read")   handleReadPin(cmd); 
    else if (cmd.getRoot() == "set")    handleSetPin(cmd);
    else if (cmd.getRoot() == "pullup") handlePullup(cmd);
    else if (cmd.getRoot() == "pwm")    handlePwm(cmd);
    else if (cmd.getRoot() == "toggle") handleTogglePin(cmd);
    else if (cmd.getRoot() == "analog") handleAnalog(cmd);
    else if (cmd.getRoot() == "reset")  handleResetPin(cmd);
    else                                handleHelp();
    
}

/*
Read
*/
void DioController::handleReadPin(const TerminalCommand& cmd) {
    if (cmd.getSubcommand().empty() || !argTransformer.isValidNumber(cmd.getSubcommand())) {
        terminalView.println("Usage: read <pin>");
        return;
    }

    uint8_t pin = argTransformer.toUint8(cmd.getSubcommand());
    if (!isPinAllowed(pin, "Read")) return;
    int value = pinService.read(pin);
    terminalView.println("Pin " + std::to_string(pin) + " = " + std::to_string(value));
}

/*
Set
*/
void DioController::handleSetPin(const TerminalCommand& cmd) {
    if (cmd.getSubcommand().empty() || !argTransformer.isValidNumber(cmd.getSubcommand())) {
        terminalView.println("Usage: set <pin> <IN/OUT/HI/LOW>");
        return;
    }

    std::string arg = cmd.getArgs();
    if (arg.empty()) {
        terminalView.println("DIO Set: Missing mode (IN/OUT/HI/LOW).");
        return;
    }

    uint8_t pin = argTransformer.toUint8(cmd.getSubcommand());
    if (!isPinAllowed(pin, "Set")) return;
    char c = std::toupper(arg[0]);

    switch (c) {
        case 'I':
            pinService.setInput(pin);
            terminalView.println("DIO Set: Pin " + std::to_string(pin) + " set to INPUT");
            break;
        case 'O':
            pinService.setOutput(pin);
            terminalView.println("DIO Set: Pin " + std::to_string(pin) + " set to OUTPUT");
            break;
        case 'H':
            pinService.setOutput(pin);
            pinService.setHigh(pin);
            terminalView.println("DIO Set: Pin " + std::to_string(pin) + " set to HIGH");
            break;
        case 'L':
            pinService.setOutput(pin);
            pinService.setLow(pin);
            terminalView.println("DIO Set: Pin " + std::to_string(pin) + " set to LOW");
            break;
        default:
            terminalView.println("Unknown command. Use I, O, H, or L.");
            break;
    }
}

/*
Pullup
*/
void DioController::handlePullup(const TerminalCommand& cmd) {
    if (cmd.getSubcommand().empty() || !argTransformer.isValidNumber(cmd.getSubcommand())) {
        terminalView.println("Usage: pullup <pin>");
        return;
    }

    int pin = argTransformer.toUint8(cmd.getSubcommand());
    if (!isPinAllowed(pin, "Pullup")) return;
    pinService.setInputPullup(pin);

    terminalView.println("DIO Pullup: Set on pin " + std::to_string(pin));
}

/*
Sniff
*/
void DioController::handleSniff(const TerminalCommand& cmd) {
    if (cmd.getSubcommand().empty() || !argTransformer.isValidNumber(cmd.getSubcommand())) {
        terminalView.println("Usage: sniff <pin>");
        return;
    }

    uint8_t pin = argTransformer.toUint8(cmd.getSubcommand());
    if (!isPinAllowed(pin, "Sniff")) return;
    pinService.setInput(pin);

    terminalView.println("DIO Sniff: Pin " + std::to_string(pin) + "... Press [ENTER] to stop");
    
    int last = pinService.read(pin);
    terminalView.println("Initial state: " + std::to_string(last));

    unsigned long lastCheck = millis();
    while (true) {
        // check ENTER press
        if (millis() - lastCheck > 10) {
            lastCheck = millis();
            char c = terminalInput.readChar();
            if (c == '\r' || c == '\n') {
                terminalView.println("DIO Sniff: Stopped.");
                break;
            }
        }

        // check pin state
        int current = pinService.read(pin);
        if (current != last) {
            std::string transition = (last == 0 && current == 1)
                ? "LOW  -> HIGH"
                : "HIGH -> LOW";
            terminalView.println("Pin " + std::to_string(pin) + ": " + transition);
            last = current;
        }
    }
}

/*
Analog
*/
void DioController::handleAnalog(const TerminalCommand& cmd) {
    if (cmd.getSubcommand().empty() || !argTransformer.isValidNumber(cmd.getSubcommand())) {
        terminalView.println("Usage: analog <pin>");
        return;
    }

    uint8_t pin = argTransformer.toUint8(cmd.getSubcommand());
    if (!isPinAllowed(pin, "Analog")) return;

    terminalView.println("DIO Analog: Pin " + std::to_string(pin) + " ... Press [ENTER] to stop\n");

    unsigned long lastSample = millis() + 1000; // start immediately
    unsigned long lastCheck = millis();

    while (true) {
        unsigned long now = millis();

        // Check [ENTER] every 10 ms
        if (now - lastCheck > 10) {
            lastCheck = now;
            char c = terminalInput.readChar();
            if (c == '\r' || c == '\n') {
                terminalView.println("\nDIO Analog: Stopped by user.");
                break;
            }
        }

        // Sample every 1000 ms
        if (now - lastSample >= 1000) {
            lastSample = now;

            int raw = pinService.readAnalog(pin);
            float voltage = (raw / 4095.0f) * 3.3f;

            std::ostringstream oss;
            oss << "   Analog pin " << static_cast<int>(pin)
                << ": " << raw
                << " (" << voltage << " V)";
            terminalView.println(oss.str());
        }
    }
}

/*
Pwm
*/
void DioController::handlePwm(const TerminalCommand& cmd) {
    auto sub = cmd.getSubcommand();
    auto args = argTransformer.splitArgs(cmd.getArgs());

    if (!sub.empty() && args.size() != 2) {
        terminalView.println("DIO PWM: Invalid syntax. Use:");
        terminalView.println("  pwm <pin> <frequency> <duty>");
        return;
    }

    if (!argTransformer.isValidNumber(sub) ||
        !argTransformer.isValidNumber(args[0]) ||
        !argTransformer.isValidNumber(args[1])) {
        terminalView.println("DIO PWM: All arguments must be valid numbers.");
        return;
    }

    uint8_t pin = argTransformer.toUint8(sub);
    if (!isPinAllowed(pin, "PWM")) return;

    uint32_t freq = argTransformer.toUint32(args[0]);
    uint8_t duty = argTransformer.toUint8(args[1]);

    if (duty > 100) {
        terminalView.println("DIO PWM: Duty cycle must be between 0 and 100.");
        return;
    }

    bool ok = pinService.setupPwm(pin, freq, duty);
    if (!ok) {
        terminalView.println("DIO PWM: Cannot generate " + std::to_string(freq) +
                            " Hz. Try a higher frequency or use toggle command.");
        return;
    }

    terminalView.println("DIO PWM: Pin " + std::to_string(pin) +
                         " (" + std::to_string(freq) + "Hz, " +
                         std::to_string(duty) + "% duty).");
}

/*
Toggle
*/
void DioController::handleTogglePin(const TerminalCommand& cmd) {
    
    auto args = argTransformer.splitArgs(cmd.getArgs());
    
    if (cmd.getSubcommand().empty() || args.empty()) {
        terminalView.println("Usage: toggle <pin> <ms>");
        return;
    }
    
    if (!argTransformer.isValidNumber(cmd.getSubcommand()) ||
    !argTransformer.isValidNumber(args[0])) {
        terminalView.println("DIO Toggle: Invalid arguments.");
        return;
    }
    
    uint8_t pin = argTransformer.toUint8(cmd.getSubcommand());
    if (!isPinAllowed(pin, "Toggle")) return;

    uint32_t intervalMs = argTransformer.toUint32(args[0]);
    
    pinService.setOutput(pin);
    bool state = false;

    terminalView.println("\nDIO Toggle: Pin " + std::to_string(pin) + " every " + std::to_string(intervalMs) + "ms...Press [ENTER] to stop.");
    terminalView.println("");

    unsigned long lastToggle = millis();
    unsigned long lastCheck = millis();

    while (true) {
        unsigned long now = millis();

        // check ENTER press every 10ms
        if (now - lastCheck > 10) {
            lastCheck = now;
            char c = terminalInput.readChar();
            if (c == '\r' || c == '\n') {
                terminalView.println("DIO Toggle: Stopped.");
                break;
            }
        }

        // toggle pin at defined interval
        if (now - lastToggle >= intervalMs) {
            lastToggle = now;
            state = !state;
            if (state) {
                pinService.setHigh(pin);
            } else {
                pinService.setLow(pin);
            }
        }
    }
}

/*
Reset
*/
void DioController::handleResetPin(const TerminalCommand& cmd) {
    if (cmd.getSubcommand().empty()) {
        terminalView.println("Usage: reset <pin>");
        return;
    }

    if (!argTransformer.isValidNumber(cmd.getSubcommand())) {
        terminalView.println("DIO Reset: Invalid pin number.");
        return;
    }

    uint8_t pin = argTransformer.toUint8(cmd.getSubcommand());
    if (!isPinAllowed(pin, "Reset")) return;

    // Detacher PWM
    ledcDetachPin(pin);

    // Reset Pullup
    pinService.setInput(pin);

    terminalView.println("DIO Reset: Pin " + std::to_string(pin) + " to INPUT (no pull-up, no PWM).");
}

/*
Help
*/
void DioController::handleHelp() {
    terminalView.println("Unknown DIO command. Usage:");
    terminalView.println("  sniff <pin>");
    terminalView.println("  read <pin>");
    terminalView.println("  set <pin> <H/L/I/O>");
    terminalView.println("  pullup <pin>");
    terminalView.println("  pwm <pin> <freq> <duty>");
    terminalView.println("  toggle <pin> <ms>");
    terminalView.println("  analog <pin>");
    terminalView.println("  reset <pin>");
}

bool DioController::isPinAllowed(uint8_t pin, const std::string& context) {
    const auto& protectedPins = state.getProtectedPins();
    if (std::find(protectedPins.begin(), protectedPins.end(), pin) != protectedPins.end()) {
        terminalView.println("DIO " + context + ": Pin " + std::to_string(pin) + " is protected and cannot be used.");
        return false;
    }
    return true;
}