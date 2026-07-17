#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Interfaces/ITerminalView.h"

class FakeTerminalView final : public ITerminalView {
public:
    std::string output;
    std::vector<std::string> printCalls;
    std::vector<std::string> printlnCalls;
    uint32_t waitPressCalls = 0;
    uint32_t clearCalls = 0;

    void initialize() override {}
    void welcome(TerminalTypeEnum&, std::string&) override {}

    void print(const std::string& text) override {
        printCalls.push_back(text);
        output += text;
    }

    void print(const uint8_t data) override {
        print(std::to_string(data));
    }

    void println(const std::string& text) override {
        printlnCalls.push_back(text);
        output += text + '\n';
    }

    void printPrompt(const std::string& mode = "HIZ") override {
        print(mode + "> ");
    }

    void waitPress() override { ++waitPressCalls; }

    void clear() override { ++clearCalls; }

    bool contains(const std::string& text) const {
        return output.find(text) != std::string::npos;
    }
};
