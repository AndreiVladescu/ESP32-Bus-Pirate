#pragma once

#ifdef DEVICE_CUSTOM

// -----------------------------------------------------------------------------
// Custom board compile-time defaults
// -----------------------------------------------------------------------------
// This file maps simple PlatformIO build flags to the generic board layer.
// Users should normally edit platformio.ini only.

#if defined(CUSTOM_DISPLAY_DRIVER_ST7789_SPI) && defined(CUSTOM_DISPLAY_DRIVER_ST7789_PARALLEL)
  #error "Select only one custom display driver: ST7789_SPI or ST7789_PARALLEL"
#endif

#ifndef CUSTOM_INPUT_BUTTON_PIN
  #define CUSTOM_INPUT_BUTTON_PIN 0
#endif

#ifndef CUSTOM_INPUT_BUTTON_ACTIVE_LOW
  #define CUSTOM_INPUT_BUTTON_ACTIVE_LOW 1
#endif

#ifndef CUSTOM_INPUT_BUTTON_PULLUP
  #define CUSTOM_INPUT_BUTTON_PULLUP 1
#endif

#ifndef CUSTOM_BOARD_POWER_PIN
  #define CUSTOM_BOARD_POWER_PIN -1
#endif

#ifndef CUSTOM_BOARD_POWER_ACTIVE_HIGH
  #define CUSTOM_BOARD_POWER_ACTIVE_HIGH 1
#endif

// -----------------------------------------------------------------------------
// Shared display options
// -----------------------------------------------------------------------------
#ifndef CUSTOM_DISPLAY_PANEL_WIDTH
  #define CUSTOM_DISPLAY_PANEL_WIDTH 170
#endif

#ifndef CUSTOM_DISPLAY_PANEL_HEIGHT
  #define CUSTOM_DISPLAY_PANEL_HEIGHT 320
#endif

#ifndef CUSTOM_DISPLAY_MEMORY_WIDTH
  #define CUSTOM_DISPLAY_MEMORY_WIDTH CUSTOM_DISPLAY_PANEL_WIDTH
#endif

#ifndef CUSTOM_DISPLAY_MEMORY_HEIGHT
  #define CUSTOM_DISPLAY_MEMORY_HEIGHT CUSTOM_DISPLAY_PANEL_HEIGHT
#endif

#ifndef CUSTOM_DISPLAY_OFFSET_X
  #define CUSTOM_DISPLAY_OFFSET_X 0
#endif

#ifndef CUSTOM_DISPLAY_OFFSET_Y
  #define CUSTOM_DISPLAY_OFFSET_Y 0
#endif

#ifndef CUSTOM_DISPLAY_ROTATION
  #define CUSTOM_DISPLAY_ROTATION 0
#endif

#ifndef CUSTOM_DISPLAY_INVERT
  #define CUSTOM_DISPLAY_INVERT 0
#endif

#ifndef CUSTOM_DISPLAY_RGB_ORDER
  #define CUSTOM_DISPLAY_RGB_ORDER 0
#endif

#ifndef CUSTOM_DISPLAY_WRITE_FREQ
  #define CUSTOM_DISPLAY_WRITE_FREQ 40000000
#endif

#ifndef CUSTOM_DISPLAY_BACKLIGHT_ACTIVE_HIGH
  #define CUSTOM_DISPLAY_BACKLIGHT_ACTIVE_HIGH 1
#endif

#ifndef CUSTOM_DISPLAY_POWER_ACTIVE_HIGH
  #define CUSTOM_DISPLAY_POWER_ACTIVE_HIGH 1
#endif

// New bus-oriented names. Legacy CUSTOM_DISPLAY_PIN_* aliases are kept so older
// local profiles still compile, but platformio.ini should use the bus names.
#ifndef CUSTOM_DISPLAY_BACKLIGHT_PIN
  #ifdef CUSTOM_DISPLAY_PIN_BACKLIGHT
    #define CUSTOM_DISPLAY_BACKLIGHT_PIN CUSTOM_DISPLAY_PIN_BACKLIGHT
  #else
    #define CUSTOM_DISPLAY_BACKLIGHT_PIN -1
  #endif
#endif

#ifndef CUSTOM_DISPLAY_POWER_PIN
  #ifdef CUSTOM_DISPLAY_PIN_POWER
    #define CUSTOM_DISPLAY_POWER_PIN CUSTOM_DISPLAY_PIN_POWER
  #else
    #define CUSTOM_DISPLAY_POWER_PIN -1
  #endif
#endif

#ifndef CUSTOM_DISPLAY_RESET_PIN
  #ifdef CUSTOM_DISPLAY_PIN_RESET
    #define CUSTOM_DISPLAY_RESET_PIN CUSTOM_DISPLAY_PIN_RESET
  #else
    #define CUSTOM_DISPLAY_RESET_PIN -1
  #endif
#endif

#ifndef CUSTOM_DISPLAY_DC_PIN
  #ifdef CUSTOM_DISPLAY_PIN_DC
    #define CUSTOM_DISPLAY_DC_PIN CUSTOM_DISPLAY_PIN_DC
  #else
    #define CUSTOM_DISPLAY_DC_PIN -1
  #endif
#endif

#ifndef CUSTOM_DISPLAY_CS_PIN
  #ifdef CUSTOM_DISPLAY_PIN_CS
    #define CUSTOM_DISPLAY_CS_PIN CUSTOM_DISPLAY_PIN_CS
  #else
    #define CUSTOM_DISPLAY_CS_PIN -1
  #endif
#endif

// -----------------------------------------------------------------------------
// Display SPI bus pins - used by SPI display drivers, currently ST7789 SPI.
// -----------------------------------------------------------------------------
#ifndef CUSTOM_DISPLAY_SPI_MISO
  #ifdef CUSTOM_DISPLAY_PIN_MISO
    #define CUSTOM_DISPLAY_SPI_MISO CUSTOM_DISPLAY_PIN_MISO
  #else
    #define CUSTOM_DISPLAY_SPI_MISO -1
  #endif
#endif

#ifndef CUSTOM_DISPLAY_SPI_MOSI
  #ifdef CUSTOM_DISPLAY_PIN_MOSI
    #define CUSTOM_DISPLAY_SPI_MOSI CUSTOM_DISPLAY_PIN_MOSI
  #else
    #define CUSTOM_DISPLAY_SPI_MOSI -1
  #endif
#endif

#ifndef CUSTOM_DISPLAY_SPI_SCLK
  #ifdef CUSTOM_DISPLAY_PIN_SCLK
    #define CUSTOM_DISPLAY_SPI_SCLK CUSTOM_DISPLAY_PIN_SCLK
  #else
    #define CUSTOM_DISPLAY_SPI_SCLK -1
  #endif
#endif

#ifndef CUSTOM_DISPLAY_SPI_READ_FREQ
  #ifdef CUSTOM_DISPLAY_READ_FREQ
    #define CUSTOM_DISPLAY_SPI_READ_FREQ CUSTOM_DISPLAY_READ_FREQ
  #else
    #define CUSTOM_DISPLAY_SPI_READ_FREQ 20000000
  #endif
#endif

#ifndef CUSTOM_DISPLAY_SPI_USE_SHARED
  #ifdef CUSTOM_DISPLAY_USE_SHARED_SPI
    #define CUSTOM_DISPLAY_SPI_USE_SHARED CUSTOM_DISPLAY_USE_SHARED_SPI
  #else
    #define CUSTOM_DISPLAY_SPI_USE_SHARED 1
  #endif
#endif

// -----------------------------------------------------------------------------
// Display 8-bit parallel bus pins - used by parallel display drivers,
// currently ST7789 parallel.
// -----------------------------------------------------------------------------
#ifndef CUSTOM_DISPLAY_PARALLEL_WR
  #ifdef CUSTOM_DISPLAY_PIN_WR
    #define CUSTOM_DISPLAY_PARALLEL_WR CUSTOM_DISPLAY_PIN_WR
  #else
    #define CUSTOM_DISPLAY_PARALLEL_WR -1
  #endif
#endif

#ifndef CUSTOM_DISPLAY_PARALLEL_RD
  #ifdef CUSTOM_DISPLAY_PIN_RD
    #define CUSTOM_DISPLAY_PARALLEL_RD CUSTOM_DISPLAY_PIN_RD
  #else
    #define CUSTOM_DISPLAY_PARALLEL_RD -1
  #endif
#endif

#ifndef CUSTOM_DISPLAY_PARALLEL_D0
  #ifdef CUSTOM_DISPLAY_PIN_D0
    #define CUSTOM_DISPLAY_PARALLEL_D0 CUSTOM_DISPLAY_PIN_D0
  #else
    #define CUSTOM_DISPLAY_PARALLEL_D0 -1
  #endif
#endif

#ifndef CUSTOM_DISPLAY_PARALLEL_D1
  #ifdef CUSTOM_DISPLAY_PIN_D1
    #define CUSTOM_DISPLAY_PARALLEL_D1 CUSTOM_DISPLAY_PIN_D1
  #else
    #define CUSTOM_DISPLAY_PARALLEL_D1 -1
  #endif
#endif

#ifndef CUSTOM_DISPLAY_PARALLEL_D2
  #ifdef CUSTOM_DISPLAY_PIN_D2
    #define CUSTOM_DISPLAY_PARALLEL_D2 CUSTOM_DISPLAY_PIN_D2
  #else
    #define CUSTOM_DISPLAY_PARALLEL_D2 -1
  #endif
#endif

#ifndef CUSTOM_DISPLAY_PARALLEL_D3
  #ifdef CUSTOM_DISPLAY_PIN_D3
    #define CUSTOM_DISPLAY_PARALLEL_D3 CUSTOM_DISPLAY_PIN_D3
  #else
    #define CUSTOM_DISPLAY_PARALLEL_D3 -1
  #endif
#endif

#ifndef CUSTOM_DISPLAY_PARALLEL_D4
  #ifdef CUSTOM_DISPLAY_PIN_D4
    #define CUSTOM_DISPLAY_PARALLEL_D4 CUSTOM_DISPLAY_PIN_D4
  #else
    #define CUSTOM_DISPLAY_PARALLEL_D4 -1
  #endif
#endif

#ifndef CUSTOM_DISPLAY_PARALLEL_D5
  #ifdef CUSTOM_DISPLAY_PIN_D5
    #define CUSTOM_DISPLAY_PARALLEL_D5 CUSTOM_DISPLAY_PIN_D5
  #else
    #define CUSTOM_DISPLAY_PARALLEL_D5 -1
  #endif
#endif

#ifndef CUSTOM_DISPLAY_PARALLEL_D6
  #ifdef CUSTOM_DISPLAY_PIN_D6
    #define CUSTOM_DISPLAY_PARALLEL_D6 CUSTOM_DISPLAY_PIN_D6
  #else
    #define CUSTOM_DISPLAY_PARALLEL_D6 -1
  #endif
#endif

#ifndef CUSTOM_DISPLAY_PARALLEL_D7
  #ifdef CUSTOM_DISPLAY_PIN_D7
    #define CUSTOM_DISPLAY_PARALLEL_D7 CUSTOM_DISPLAY_PIN_D7
  #else
    #define CUSTOM_DISPLAY_PARALLEL_D7 -1
  #endif
#endif

#ifndef CUSTOM_DISPLAY_PARALLEL_BUSY
  #ifdef CUSTOM_DISPLAY_PIN_BUSY
    #define CUSTOM_DISPLAY_PARALLEL_BUSY CUSTOM_DISPLAY_PIN_BUSY
  #else
    #define CUSTOM_DISPLAY_PARALLEL_BUSY -1
  #endif
#endif

#ifndef CUSTOM_DISPLAY_PARALLEL_DLEN_16BIT
  #ifdef CUSTOM_DISPLAY_DLEN_16BIT
    #define CUSTOM_DISPLAY_PARALLEL_DLEN_16BIT CUSTOM_DISPLAY_DLEN_16BIT
  #else
    #define CUSTOM_DISPLAY_PARALLEL_DLEN_16BIT 0
  #endif
#endif

#ifndef CUSTOM_DISPLAY_HELP_LINE_1
  #define CUSTOM_DISPLAY_HELP_LINE_1 "Press BOOT to change terminal"
#endif

#ifndef CUSTOM_DISPLAY_HELP_LINE_2
  #define CUSTOM_DISPLAY_HELP_LINE_2 "Wait / press BOOT to accept"
#endif

#endif // DEVICE_CUSTOM
