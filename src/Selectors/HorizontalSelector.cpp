#include "HorizontalSelector.h"

HorizontalSelector::HorizontalSelector(IDeviceView& display, IInput& input)
    : display(display), input(input) {}

int HorizontalSelector::select(
    const std::string& title, 
    const std::vector<std::string>& options, 
    const std::string& description1, 
    const std::string& description2) {

    int currentIndex = 0;
    int lastIndex = -1;

    display.topBar(title, false, false);

    while (true) {
        if (lastIndex != currentIndex) {
            display.horizontalSelection(options, currentIndex, description1, description2);
            lastIndex = currentIndex;
        }

        char key = input.handler();

        switch (key) {
            case KEY_ARROW_LEFT:
                currentIndex = (currentIndex > 0) ? currentIndex - 1 : options.size() - 1;
                break;
             case KEY_ARROW_RIGHT:
            #if !defined(DEVICE_TDISPLAYS3)                
               currentIndex = (currentIndex < options.size() - 1) ? currentIndex + 1 : 0;
                break;
            case KEY_OK:
            #endif

                return currentIndex;
            default:
                break;
        }
    }
}

int HorizontalSelector::selectHeadless() {
    std::vector<std::string> options = {
        TerminalTypeEnumMapper::toString(TerminalTypeEnum::WiFiClient),
        TerminalTypeEnumMapper::toString(TerminalTypeEnum::WiFiAp),
        TerminalTypeEnumMapper::toString(TerminalTypeEnum::SerialPort),
    };

    int selected = 2;  // default: Serial
    const unsigned long longPressMs = 800;

    display.topBar("ESP32 BIT PIRATE", false, false);
    display.horizontalSelection(
        options,
        selected,
        "Terminal auto-select",
        "Short press: CONNECT  Long press: HOTSPOT"
    );

    // 3 sec window:
    // - no input: Serial
    // - short press: WiFi Connect
    // - long press: WiFi Hotspot
    const unsigned long timeout = millis() + 3000;
    while (millis() < timeout) {
        char c = input.readChar();
        if (c == KEY_OK) {
            const unsigned long pressStart = millis();
            while (input.readChar() == KEY_OK) {
                if (millis() - pressStart >= longPressMs) {
                    selected = 1; // WiFi Hotspot
                    display.horizontalSelection(
                        options,
                        selected,
                        "Terminal selected",
                        "Starting hotspot..."
                    );
                    delay(250);
                    return selected;
                }
                delay(10);
            }

            selected = 0; // WiFi Connect
            display.horizontalSelection(
                options,
                selected,
                "Terminal selected",
                "Connecting to WiFi..."
            );
            delay(250);
            break;
        }
        delay(10);
    }

    return selected;
}
