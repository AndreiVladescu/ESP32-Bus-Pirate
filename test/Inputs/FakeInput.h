#pragma once

#include <cstdint>
#include <deque>
#include <string>

#include "Interfaces/IInput.h"

class FakeInput final : public IInput {
public:
    std::deque<char> blockingChars;
    std::deque<char> nonBlockingChars;
    uint32_t waitPressCalls = 0;
    uint32_t lastWaitTimeoutMs = 0;

    char handler() override {
        return popOrEnter(blockingChars);
    }

    char readChar() override {
        return popOrEnter(nonBlockingChars);
    }

    void waitPress(uint32_t timeoutMs = 0) override {
        ++waitPressCalls;
        lastWaitTimeoutMs = timeoutMs;
    }

    void queueLine(const std::string& line) {
        for (char ch : line) blockingChars.push_back(ch);
        blockingChars.push_back('\n');
    }

    void queueReadChar(char ch) {
        nonBlockingChars.push_back(ch);
    }

private:
    static char popOrEnter(std::deque<char>& chars) {
        if (chars.empty()) return '\n';
        const char ch = chars.front();
        chars.pop_front();
        return ch;
    }
};
