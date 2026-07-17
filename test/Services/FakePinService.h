#pragma once

#include <cstdint>
#include <deque>
#include <map>
#include <vector>

#include "Interfaces/IPinService.h"

class FakePinService final : public IPinService {
public:
    std::vector<uint8_t> inputCalls;
    std::vector<uint8_t> inputPullupCalls;
    std::vector<uint8_t> inputPulldownCalls;
    std::vector<uint8_t> outputCalls;
    std::vector<uint8_t> highCalls;
    std::vector<uint8_t> lowCalls;
    std::vector<uint8_t> readCalls;
    std::vector<uint8_t> detachCalls;
    std::deque<bool> readValues;
    std::map<uint8_t, bool> inputModes;
    std::map<uint8_t, pullType> pulls;
    std::vector<uint8_t> configuredPullPins;
    bool defaultRead = false;
    int analogValue = 0;
    bool pwmResult = true;
    bool servoResult = true;
    uint8_t lastPwmPin = 0;
    uint32_t lastPwmFrequency = 0;
    uint8_t lastPwmDuty = 0;
    uint32_t pwmCalls = 0;
    uint8_t lastServoPin = 0;
    uint8_t lastServoAngle = 0;
    uint32_t servoCalls = 0;

    void setInput(uint8_t pin) override { inputCalls.push_back(pin); inputModes[pin] = true; pulls[pin] = NOPULL; }
    void setInputPullup(uint8_t pin) override { inputPullupCalls.push_back(pin); inputModes[pin] = true; pulls[pin] = PULL_UP; }
    void setInputPullDown(uint8_t pin) override { inputPulldownCalls.push_back(pin); inputModes[pin] = true; pulls[pin] = PULL_DOWN; }
    void setOutput(uint8_t pin) override { outputCalls.push_back(pin); inputModes[pin] = false; }
    void setHigh(uint8_t pin) override { highCalls.push_back(pin); }
    void setLow(uint8_t pin) override { lowCalls.push_back(pin); }

    bool read(uint8_t pin) override {
        readCalls.push_back(pin);
        if (readValues.empty()) return defaultRead;
        const bool value = readValues.front();
        readValues.pop_front();
        return value;
    }

    bool isInputMode(uint8_t pin) override {
        auto it = inputModes.find(pin);
        return it == inputModes.end() ? true : it->second;
    }

    void togglePullup(uint8_t pin) override { pulls[pin] = PULL_UP; }
    void togglePullDown(uint8_t pin) override { pulls[pin] = PULL_DOWN; }
    int readAnalog(uint8_t) override { return analogValue; }

    bool setupPwm(uint8_t pin, uint32_t freq, uint8_t dutyPercent) override {
        ++pwmCalls;
        lastPwmPin = pin;
        lastPwmFrequency = freq;
        lastPwmDuty = dutyPercent;
        return pwmResult;
    }

    bool setServoAngle(uint8_t pin, uint8_t angle) override {
        ++servoCalls;
        lastServoPin = pin;
        lastServoAngle = angle;
        return servoResult;
    }

    pullType getPullType(uint8_t pin) override {
        auto it = pulls.find(pin);
        return it == pulls.end() ? NOPULL : it->second;
    }

    std::vector<uint8_t> getConfiguredPullPins() override { return configuredPullPins; }
    void detachSignal(uint8_t pin) override { detachCalls.push_back(pin); }
};
