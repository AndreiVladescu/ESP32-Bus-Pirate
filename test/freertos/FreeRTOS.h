#pragma once

#include <cstdint>

using TickType_t = uint32_t;
using TaskHandle_t = void*;
using portMUX_TYPE = int;

#ifndef portMUX_INITIALIZER_UNLOCKED
#define portMUX_INITIALIZER_UNLOCKED 0
#endif

#ifndef pdMS_TO_TICKS
#define pdMS_TO_TICKS(ms) (static_cast<TickType_t>(ms))
#endif

inline void vTaskDelay(TickType_t) {}

