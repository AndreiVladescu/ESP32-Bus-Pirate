#pragma once

#include <Arduino.h>
#include <unordered_map>
#include <vector>

class PinService {
public:
    void setInput(uint8_t pin);
    void setInputPullup(uint8_t pin);
    void setInputPullDown(uint8_t pin);
    void setOutput(uint8_t pin);
    void setHigh(uint8_t pin);
    void setLow(uint8_t pin);
    bool read(uint8_t pin);
    bool isInputMode(uint8_t pin);
    void togglePullup(uint8_t pin);
    void togglePullDown(uint8_t pin);
    int readAnalog(uint8_t pin);
    bool setupPwm(uint8_t pin, uint32_t freq, uint8_t dutyPercent);
    bool setServoAngle(uint8_t pin, uint8_t angle);
    typedef enum  pullType { NOPULL = 0, PULL_UP, PULL_DOWN}  pullType;
    pullType getPullType(uint8_t pin);
    std::vector<uint8_t> getConfiguredPullPins();
    void detachSignal(uint8_t pin);
private:
    // track pins with pull config 
    std::unordered_map<uint8_t, pullType> pullState; 

    // track pins with active PWM 
    std::vector<uint8_t> activePwmPins;

    bool isActivePwmPin(uint8_t pin) const;
    void markActivePwmPin(uint8_t pin);
    void unmarkActivePwmPin(uint8_t pin);
};
