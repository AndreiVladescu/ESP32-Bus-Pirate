#include "UserInputManager.h"

std::string UserInputManager::getLine() {
    std::string result;
    size_t cursorIndex = 0;

    while (true) {
        char c = terminalInput.handler();

        // Enter
        if (c == '\r' || c == '\n') break;

        // Backspace
        if ((c == '\b' || c == 127) && cursorIndex > 0) {
            cursorIndex--;
            result.erase(cursorIndex, 1);
            terminalView.print("\b \b");
            continue;
        }

        // Printable
        if (isprint(c)) {
            result.insert(cursorIndex, 1, c);
            cursorIndex++;
            terminalView.print(std::string(1, c));
        }
    }

    terminalView.println("");
    return result;
}

uint8_t UserInputManager::readValidatedUint8(const std::string& label, uint8_t def, uint8_t min, uint8_t max) {
    while (true) {
        terminalView.print(label + " [" + std::to_string(def) + "]: ");
        std::string input = getLine();
        if (input.empty()) return def;

        if (argTransformer.isValidNumber(input)) {
            uint8_t val = argTransformer.toUint8(input);
            if (val >= min && val <= max) return val;
        }

        terminalView.println("Invalid input. Must be " + std::to_string(min) + "-" + std::to_string(max));
    }
}

uint8_t UserInputManager::readValidatedUint8(const std::string& label, uint8_t defaultVal) {
    return readValidatedUint8(label, defaultVal, 0, 255);
}

uint32_t UserInputManager::readValidatedUint32(const std::string& label, uint32_t def) {
    while (true) {
        terminalView.print(label + " [" + std::to_string(def) + "]: ");
        std::string input = getLine();
        if (input.empty()) return def;

        if (argTransformer.isValidNumber(input)) {
            return argTransformer.toUint32(input);
        }

        terminalView.println("Invalid number.");
    }
}

char UserInputManager::readCharChoice(const std::string& label, char def, const std::vector<char>& allowed) {
    while (true) {
        terminalView.print(label + " [" + def + "]: ");
        std::string input = getLine();
        if (input.empty()) return def;

        char c = toupper(input[0]);
        if (std::find(allowed.begin(), allowed.end(), c) != allowed.end()) return c;

        terminalView.println("Invalid choice.");
    }
}

bool UserInputManager::readYesNo(const std::string& label, bool def) {
    while (true) {
        terminalView.print(label + " [" + (def ? "y" : "n") + "]: ");
        std::string input = getLine();
        if (input.empty()) return def;

        char c = tolower(input[0]);
        if (c == 'y') return true;
        if (c == 'n') return false;

        terminalView.println("Please answer y or n.");
    }
}

uint8_t UserInputManager::readModeNumber() {
    std::string inputDigit;
    size_t cursorIndex = 0;

    while (true) {
        char c = terminalInput.handler();

        // Enter
        if (c == '\r' || c == '\n') {
            terminalView.println("");
            break;
        }

        // Backspace
        if ((c == '\b' || c == 127) && cursorIndex > 0) {
            cursorIndex--;
            inputDigit.erase(cursorIndex, 1);
            terminalView.print("\b \b");
            continue;
        }

        // Only number
        if (std::isdigit(c)) {
            inputDigit.insert(cursorIndex, 1, c);
            cursorIndex++;
            terminalView.print(std::string(1, c));
        }
    }

    if (inputDigit.empty()) {
        return -1;
    }

    return std::stoi(inputDigit);
}

uint8_t UserInputManager::readValidatedPinNumber(const std::string& label, uint8_t def, uint8_t min, uint8_t max, const std::vector<uint8_t>& forbiddenPins) {
    while (true) {
        int val = readValidatedUint8(label, def, min, max);
        if (std::find(forbiddenPins.begin(), forbiddenPins.end(), val) != forbiddenPins.end()) {
            terminalView.println("This pin is reserved/protected and cannot be used.");
            continue;
        }
        return val;
    }
}

uint8_t UserInputManager::readValidatedPinNumber(const std::string& label, uint8_t def, const std::vector<uint8_t>& forbiddenPins) {
    return readValidatedPinNumber(label, def, 0, 48, forbiddenPins);
}

std::vector<uint8_t> UserInputManager::readValidatedPinGroup(
    const std::string& label,
    const std::vector<uint8_t>& defaultPins,
    const std::vector<uint8_t>& protectedPins
) {
    while (true) {
        // Default pins
        std::string defaultStr;
        for (size_t i = 0; i < defaultPins.size(); ++i) {
            if (i > 0) defaultStr += " ";
            defaultStr += std::to_string(defaultPins[i]);
        }
        
        // Display default list [1 2 3 ...]
        terminalView.print(label + " [" + defaultStr + "]: ");

        // Get user input
        std::string input = getLine();

        // Empty, we keep default pins
        if (input.empty()) {
            return defaultPins;
        }

        std::stringstream ss(input);
        std::vector<uint8_t> pins;
        int val;
        
        // Validate
        bool valid = true;
        while (ss >> val) {
            // Invalid
            if (val < 0 || val > 48) {
                terminalView.println("Invalid pin: " + std::to_string(val));
                valid = false;
                break;
            }
            // Protected
            if (std::find(protectedPins.begin(), protectedPins.end(), val) != protectedPins.end()) {
                terminalView.println("Pin " + std::to_string(val) + " is protected/reserved.");
                valid = false;
                break;
            }
            pins.push_back(static_cast<uint8_t>(val));
        }

        // Valid new pins
        if (valid && !pins.empty()) {
            return pins;
        }

        terminalView.println("Please enter valid, non-protected GPIOs separated by spaces.");
    }
}

std::string UserInputManager::readValidatedHexString(const std::string& label, size_t numBytes, bool ignoreLen) {
    while (true) {
        terminalView.print(label + "(hex): ");
        std::string input = getLine();

        // Erase space if any
        input.erase(std::remove_if(input.begin(), input.end(), ::isspace), input.end());

        // Verify length
        if (!ignoreLen && input.length() != numBytes * 2) {
            terminalView.println("❌ Invalid length. Expected " + std::to_string(numBytes * 2) + " hex digits.");
            continue;
        }

        // If ignoring length, ensure at least 1 hex char
        if (ignoreLen && input.empty()) {
            terminalView.println("❌ Input cannot be empty.");
            continue;
        }

        // Verify each char as hexadecimal
        bool valid = std::all_of(input.begin(), input.end(), [](char c) {
            return std::isxdigit(static_cast<unsigned char>(c));
        });

        if (!valid) {
            terminalView.println("❌ Invalid characters. Only hexadecimal digits (0-9, A-F) are allowed.");
            continue;
        }

        // Add spaces
        std::string spaced;
        for (size_t i = 0; i < input.length(); i += 2) {
            if (i > 0) spaced += ' ';
            spaced += input.substr(i, 2);
        }

        return spaced;
    }
}

uint16_t UserInputManager::readValidatedCanId(const std::string& label, uint16_t defaultValue) {
    while (true) {
        terminalView.print(label + " (hex, max 3 digits) [default: " + argTransformer.toHex(defaultValue, 3) + "]: ");
        std::string input = getLine();

        if (input.empty()) return defaultValue;

        // Remove spaces
        input.erase(std::remove_if(input.begin(), input.end(), ::isspace), input.end());

        // Allow "0x" prefix
        if (input.rfind("0x", 0) == 0 || input.rfind("0X", 0) == 0) {
            input = input.substr(2);  // supprime le "0x"
        }

        // Check if input is valid hex
        bool valid = std::all_of(input.begin(), input.end(), [](char c) {
            return std::isxdigit(static_cast<unsigned char>(c));
        });

        if (!valid) {
            terminalView.println("❌ Invalid characters. Use hex digits only (0-9, A-F).");
            continue;
        }

        // Check length
        if (input.length() > 3) {
            terminalView.println("❌ Too long. Standard CAN ID must be ≤ 0x7FF (3 hex digits).");
            continue;
        }

        // Convert
        uint16_t id = std::stoul(input, nullptr, 16);

        // Check max value
        if (id > 0x7FF) {
            terminalView.println("❌ Value exceeds standard 11-bit CAN ID (max 0x7FF).");
            continue;
        }

        return id;
    }
}
