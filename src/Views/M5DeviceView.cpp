#if defined(DEVICE_CARDPUTER) || defined(DEVICE_M5STICK)

#include "M5DeviceView.h"


void M5DeviceView::initialize() {
    M5.Lcd.fillScreen(BACKGROUND_COLOR);
}

void M5DeviceView::welcome(TerminalTypeEnum& terminalType, std::string& terminalInfos) {
    if (terminalType == TerminalTypeEnum::Serial) {
        welcomeSerial(terminalInfos);
    } else {
        welcomeWeb(terminalInfos);
    }
}

void M5DeviceView::logo() {
    clear();

    M5.Lcd.fillRoundRect(8, 40, 220, 60, DEFAULT_ROUND_RECT, RECT_COLOR_DARK);
    M5.Lcd.drawRoundRect(8, 40, 220, 60, DEFAULT_ROUND_RECT, PRIMARY_COLOR);

    M5.Lcd.setTextSize(3);
    M5.Lcd.setTextColor(PRIMARY_COLOR);
    M5.Lcd.setCursor(31, 49);
    M5.Lcd.printf("Bus Pirate");

    M5.Lcd.setTextSize(1.6);
    M5.Lcd.setCursor(43, 80);
    M5.Lcd.setTextColor(TEXT_COLOR);
    M5.Lcd.printf("Version 0.5 - Geo");

    delay(3000);
}

void M5DeviceView::show(PinoutConfig& config) {
    clear();

    const auto& mappings = config.getMappings();
    auto mode = config.getMode();


    M5.Lcd.setTextSize(2);

    if (mappings.size() > 4) {
        showDetailedConfig(config, 255);  // pas de selec active
        return;
    }

    showModeName(mode, 5);

    if (mappings.empty()) {
        noMapping();
        return;
    }

    int boxHeight = 24;
    int startY = 25;
    int spacing = 4;

    for (size_t i = 0; i < mappings.size(); ++i) {
        const std::string& mapping = mappings[i];
        int y = startY + i * (boxHeight + spacing);

        M5.Lcd.fillRoundRect(DEFAULT_MARGIN, y, M5.Lcd.width() - 2 * DEFAULT_MARGIN, boxHeight, DEFAULT_ROUND_RECT, RECT_COLOR_DARK);
        M5.Lcd.drawRoundRect(DEFAULT_MARGIN, y, M5.Lcd.width() - 2 * DEFAULT_MARGIN, boxHeight, DEFAULT_ROUND_RECT, PRIMARY_COLOR);

        int16_t textX = (M5.Lcd.width() - M5.Lcd.textWidth(mapping.c_str())) / 2;
        int16_t textY = y + (boxHeight - M5.Lcd.fontHeight()) / 2 + 1;
        M5.Lcd.setCursor(textX, textY);
        M5.Lcd.setTextColor(TEXT_COLOR);
        M5.Lcd.print(mapping.c_str());
    }
}

void M5DeviceView::topBar(const std::string& title, bool submenu, bool searchBar) {
   M5.Lcd.fillRect(0, 0, M5.Lcd.width(), TOP_BAR_SIZE, BACKGROUND_COLOR);

   M5.Lcd.setTextSize(2);
   M5.Lcd.setTextColor(TEXT_COLOR);

    std::string text = title;
    if (searchBar) {
        text = title.empty() ? "Type to search" : title;
    }

   int x = (M5.Lcd.width() -M5.Lcd.textWidth(text.c_str())) / 2;
   M5.Lcd.setCursor(x, 16);
   M5.Lcd.printf(text.c_str());
}

void M5DeviceView::horizontalSelection(
    const std::vector<std::string>& options,
    uint16_t selectedIndex,
    const std::string& description1,
    const std::string& description2) {

    M5.Lcd.fillRect(0, TOP_BAR_SIZE, M5.Lcd.width(),M5.Lcd.height(), BACKGROUND_COLOR);

    // Description 1
    M5.Lcd.setTextSize(1.7);
    M5.Lcd.setTextColor(PRIMARY_COLOR);
    M5.Lcd.setCursor((M5.Lcd.width() -M5.Lcd.textWidth(description1.c_str())) / 2, 46);
    M5.Lcd.printf(description1.c_str());

    // Description 2
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor((M5.Lcd.width() -M5.Lcd.textWidth(description2.c_str())) / 2, 113);
    M5.Lcd.printf(description2.c_str());

    // Option sélectionnée (nom)
    const std::string& option = options[selectedIndex];
    M5.Lcd.fillRoundRect(40, 72, M5.Lcd.width() - 80, 50, DEFAULT_ROUND_RECT, RECT_COLOR_DARK);
    M5.Lcd.drawRoundRect(40, 72, M5.Lcd.width() - 80, 50, DEFAULT_ROUND_RECT, RECT_COLOR_LIGHT);

    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(TEXT_COLOR);
    M5.Lcd.setCursor((M5.Lcd.width() -M5.Lcd.textWidth(option.c_str())) / 2, 90);
    M5.Lcd.printf(option.c_str());

    // Flèches gauche/droite
    M5.Lcd.setCursor(15, 91);
    M5.Lcd.printf("<");
    M5.Lcd.setCursor(M5.Lcd.width() - 27, 91);
    M5.Lcd.printf(">");
}

void M5DeviceView::welcomeSerial(const std::string& baudStr) {
    M5.Lcd.fillScreen(BACKGROUND_COLOR);

    // Top
    M5.Lcd.setCursor(14, 28);
    M5.Lcd.setTextSize(1.8);
    M5.Lcd.setTextColor(TEXT_COLOR);
    M5.Lcd.println("Open Serial (USB COM)");

    // Baud
    M5.Lcd.setTextSize(2);
    M5.Lcd.fillRoundRect(8, 50, 220, 40, DEFAULT_ROUND_RECT, RECT_COLOR_DARK);
    M5.Lcd.drawRoundRect(8, 50, 220, 40, DEFAULT_ROUND_RECT, PRIMARY_COLOR);
    M5.Lcd.setTextColor(TEXT_COLOR);
    auto baud = "Baudrate: " + baudStr;
    int16_t x = (M5.Lcd.width() - M5.Lcd.textWidth(baud.c_str())) / 2;
    M5.Lcd.setCursor(x, 63);
    M5.Lcd.printf(baud.c_str());
    
    // Sub
    M5.Lcd.setTextSize(1.5);
    M5.Lcd.setCursor(8, 100);
    M5.Lcd.println("Then press a key to start");
}

void M5DeviceView::welcomeWeb(const std::string& ipStr) {
    M5.Lcd.fillScreen(BACKGROUND_COLOR);

    M5.Lcd.setCursor(16, 34);
    M5.Lcd.setTextSize(1.6);
    M5.Lcd.setTextColor(TEXT_COLOR);
    M5.Lcd.println("Open browser to connect");

    M5.Lcd.setTextSize(1.8);
    M5.Lcd.fillRoundRect(8, 60, 220, 40, DEFAULT_ROUND_RECT, RECT_COLOR_DARK);
    M5.Lcd.drawRoundRect(8, 60, 220, 40, DEFAULT_ROUND_RECT, PRIMARY_COLOR);

    M5.Lcd.setTextColor(TEXT_COLOR);
    auto  ip = "http://" + ipStr;
    int16_t x = (M5.Lcd.width() - M5.Lcd.textWidth(ip.c_str())) / 2;
    M5.Lcd.setCursor(x, 73);
    M5.Lcd.printf( ip.c_str());
}

void M5DeviceView::clear() {
    M5.Lcd.fillScreen(BACKGROUND_COLOR);
}

void M5DeviceView::setRotation(uint8_t rotation) {
    M5.Lcd.setRotation(rotation);
}

void M5DeviceView::showModeName(std::string& mode, int y) {
    auto modeName = "Mode: " + mode;
    int16_t titleX = (M5.Lcd.width() - M5.Lcd.textWidth(modeName.c_str())) / 2;
    M5.Lcd.setTextColor(TEXT_COLOR, BLACK);
    M5.Lcd.setCursor(titleX, y);
    M5.Lcd.print("Mode: ");
    M5.Lcd.setTextColor(PRIMARY_COLOR, BLACK);
    M5.Lcd.print(mode.c_str());

}

void M5DeviceView::noMapping() {
    // Box frame
    M5.Lcd.drawRoundRect(10, 28, M5.Lcd.width() - 20, 97, DEFAULT_ROUND_RECT, PRIMARY_COLOR);

    // Description
    std::string map = "No mapping defined";
    M5.Lcd.setTextSize(1.5);
    M5.Lcd.setTextColor(TEXT_COLOR);
    int16_t titleX = (M5.Lcd.width() - M5.Lcd.textWidth(map.c_str())) / 2;
    M5.Lcd.setCursor(titleX, 70);
    M5.Lcd.printf(map.c_str());
}

void M5DeviceView::showDetailedConfig(const PinoutConfig& config, int selectedIndex) {
    clear();

    const auto& mappings = config.getMappings();
    auto mode = config.getMode();
    const size_t count = mappings.size();

    uint8_t halfWidth = (M5.Lcd.width() - 20) / 2;
    uint8_t sizeY = 22;
    uint8_t startY = 10;
    uint8_t stepY = 26;
    uint8_t margin = DEFAULT_MARGIN;
    uint8_t textOffsetY = 13;
    bool selected;

    M5.Lcd.setTextSize(2);
    showModeName(mode, M5.Lcd.height() - 15);

    M5.Lcd.setTextSize(1);

    for (size_t i = 0; i < count; ++i) {
        selected = (i == selectedIndex);

        uint8_t col = i / 4; // gauche (0) ou droite (1)
        uint8_t xPos = margin + col * (halfWidth + margin);
        uint8_t yPos = startY + (i % 4) * stepY;

        drawRect(selected, xPos, yPos, halfWidth, sizeY);
        M5.Lcd.setCursor(xPos + margin + 5, yPos + textOffsetY);
        M5.Lcd.setTextColor(TEXT_COLOR);
        M5.Lcd.print(mappings[i].c_str());
    }
}

void M5DeviceView::drawRect(bool selected, uint8_t margin, uint16_t startY, uint16_t sizeX, uint16_t sizeY) {
    // Draw rect
    if (selected) {
       M5.Lcd.fillRoundRect(margin, startY, sizeX, sizeY, DEFAULT_ROUND_RECT, PRIMARY_COLOR);
       M5.Lcd.setTextColor(TEXT_COLOR);
    } else {
       M5.Lcd.fillRoundRect(margin, startY, sizeX, sizeY, DEFAULT_ROUND_RECT, RECT_COLOR_DARK);
       M5.Lcd.drawRoundRect(margin, startY, sizeX, sizeY, DEFAULT_ROUND_RECT, PRIMARY_COLOR);
       M5.Lcd.setTextColor(TEXT_COLOR);
    }
}

void M5DeviceView::loading() {
    clear();
    M5.Lcd.setTextSize(1.5);
    M5.Lcd.fillRoundRect(20, 20, M5.Lcd.width() - 40, M5.Lcd.height() - 40, 5, RECT_COLOR_DARK);
    M5.Lcd.drawRoundRect(20, 20, M5.Lcd.width() - 40, M5.Lcd.height() - 40, 5, PRIMARY_COLOR);
    M5.Lcd.setTextColor(TEXT_COLOR);
    M5.Lcd.drawString("Loading...", 75, 60);
}

void M5DeviceView::drawLogicTrace(uint8_t pin, const std::vector<uint8_t>& buffer) {
    static constexpr int canvasWidth = 240;
    static constexpr int canvasHeight = 65;
    static constexpr int midY = canvasHeight / 2;

    // Canvas
    M5Canvas canvas(&M5.Lcd);
    canvas.setColorDepth(8);
    canvas.createSprite(canvasWidth, canvasHeight);
    canvas.fillSprite(BACKGROUND_COLOR);

    // Trace
    for (size_t i = 1; i < buffer.size() && i < canvasWidth; ++i) {
        int x0 = i - 1;
        int x1 = i;
        int amplitude = 20;
        int y0 = buffer[i - 1] ? midY - amplitude : midY + amplitude;
        int y1 = buffer[i]     ? midY - amplitude : midY + amplitude;

        canvas.drawLine(x0, y0, x1, y1, PRIMARY_COLOR);
    }

    // Pin num
    canvas.drawString("Pin " + String(pin), 5, 0);

    // Center
    int x = (M5.Lcd.width() - canvasWidth) / 2;
    int y = 60;  // vertical offset
    canvas.pushSprite(x, y);

    canvas.deleteSprite();
}

#endif