#pragma once

#include <cstdint>

struct CRGB {
    uint8_t r;
    uint8_t g;
    uint8_t b;

    constexpr CRGB(uint8_t red = 0, uint8_t green = 0, uint8_t blue = 0)
        : r(red), g(green), b(blue) {}

    static const CRGB Black;
};

inline const CRGB CRGB::Black{0, 0, 0};

enum ESPIChipsets {
    APA102,
    APA102HD,
    DOTSTAR,
    DOTSTARHD,
    LPD6803,
    LPD8806,
    WS2801,
    WS2803,
    P9813,
    SK9822,
    SK9822HD,
    HD107,
    HD107HD,
};
