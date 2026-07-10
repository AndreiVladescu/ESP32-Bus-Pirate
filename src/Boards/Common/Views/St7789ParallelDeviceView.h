#pragma once

#if defined(DEVICE_TDISPLAYS3) || (defined(DEVICE_CUSTOM) && defined(CUSTOM_DISPLAY_DRIVER_ST7789_PARALLEL))

#include "Interfaces/IDeviceView.h"
#include "States/GlobalState.h"

#include <Arduino.h>
#include <LovyanGFX.hpp>

#define DARK_GREY_RECT 0x4208
#define HELP_COLOR 0xC618

// Hardware description for any 8-bit parallel ST7789 panel.
struct St7789ParallelConfig {
  int8_t pinBacklight = -1;
  int8_t pinPower = -1;

  int8_t pinWr = -1;
  int8_t pinRd = -1;
  int8_t pinDc = -1;
  int8_t pinD0 = -1;
  int8_t pinD1 = -1;
  int8_t pinD2 = -1;
  int8_t pinD3 = -1;
  int8_t pinD4 = -1;
  int8_t pinD5 = -1;
  int8_t pinD6 = -1;
  int8_t pinD7 = -1;

  int8_t pinCs = -1;
  int8_t pinReset = -1;
  int8_t pinBusy = -1;

  uint16_t panelWidth = 170;
  uint16_t panelHeight = 320;
  uint16_t memoryWidth = 240;
  uint16_t memoryHeight = 320;
  uint16_t offsetX = 35;
  uint16_t offsetY = 0;

  uint32_t writeFrequency = 20000000;
  uint8_t rotation = 3;
  bool invert = true;
  bool rgbOrder = false;
  bool dlen16Bit = false;
  bool powerActiveHigh = true;
  bool backlightActiveHigh = true;

  const char* selectionHelpLine1 = nullptr;
  const char* selectionHelpLine2 = nullptr;
};

// LovyanGFX driver for ST7789 panels using an 8-bit parallel bus.
class LGFX_ST7789Parallel : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789  _panel;
  lgfx::Bus_Parallel8 _bus;
  lgfx::Light_PWM     _light;

public:
  explicit LGFX_ST7789Parallel(const St7789ParallelConfig& displayConfig) {
    {
      auto cfg = _bus.config();

      cfg.freq_write = displayConfig.writeFrequency;
      cfg.pin_wr = displayConfig.pinWr;
      cfg.pin_rd = displayConfig.pinRd;
      cfg.pin_rs = displayConfig.pinDc;
      cfg.pin_d0 = displayConfig.pinD0;
      cfg.pin_d1 = displayConfig.pinD1;
      cfg.pin_d2 = displayConfig.pinD2;
      cfg.pin_d3 = displayConfig.pinD3;
      cfg.pin_d4 = displayConfig.pinD4;
      cfg.pin_d5 = displayConfig.pinD5;
      cfg.pin_d6 = displayConfig.pinD6;
      cfg.pin_d7 = displayConfig.pinD7;

      _bus.config(cfg);
      _panel.setBus(&_bus);
    }

    {
      auto cfg = _panel.config();

      cfg.pin_cs   = displayConfig.pinCs;
      cfg.pin_rst  = displayConfig.pinReset;
      cfg.pin_busy = displayConfig.pinBusy;

      cfg.panel_width  = displayConfig.panelWidth;
      cfg.panel_height = displayConfig.panelHeight;
      cfg.memory_width  = displayConfig.memoryWidth;
      cfg.memory_height = displayConfig.memoryHeight;
      cfg.offset_x = displayConfig.offsetX;
      cfg.offset_y = displayConfig.offsetY;

      cfg.invert = displayConfig.invert;
      cfg.rgb_order = displayConfig.rgbOrder;
      cfg.dlen_16bit = displayConfig.dlen16Bit;

      _panel.config(cfg);
    }

    setPanel(&_panel);
  }
};

class St7789ParallelDeviceView : public IDeviceView {
public:
  explicit St7789ParallelDeviceView(const St7789ParallelConfig& config);

  void initialize() override;
  SPIClass& getSharedSpiInstance() override;
  void* getScreen() override;
  void logo() override;
  void welcome(TerminalTypeEnum& terminalType, std::string& terminalInfos) override;
  void show(PinoutConfig& config) override;
  void loading() override;
  void adapterMode(const std::string& adapterName, const std::string& description, const std::vector<std::string>& details) override;
  void clear() override;
  void drawLogicTrace(uint8_t pin, const std::vector<uint8_t>& buffer, uint8_t step) override;
  void drawAnalogicTrace(uint8_t pin, const std::vector<uint8_t>& buffer, uint8_t step) override;
  void drawWaterfall(const std::string& title, float startValue, float endValue, const char* unit, int rowIndex, int rowCount, int level) override;
  void setRotation(uint8_t rotation) override;
  void setBrightness(uint8_t brightness) override;
  uint8_t getBrightness() override;
  void topBar(const std::string& title, bool submenu, bool searchBar) override;
  void horizontalSelection(
    const std::vector<std::string>& options,
    uint16_t selectedIndex,
    const std::string& description1,
    const std::string& description2
  ) override;

  void shutDown();

private:
  St7789ParallelConfig config;
  LGFX_ST7789Parallel tft;
  uint8_t brightnessPct = 100;
  SPIClass sharedSpi{HSPI};

  void drawCenterText(const std::string& text, int y, int fontSize);
  void welcomeWeb(const std::string& ip);
  void welcomeHotspot(const std::string& ip);
  void welcomeSerial(const std::string& baud);
};

#endif
