#include <unity.h>

#include "Data/InputKeys.h"
#include "../Inputs/FakeInput.h"
#include "../Services/FakeUtilityService.h"
#include "../Views/FakeDeviceView.h"
#include "Selectors/HorizontalSelector.h"

namespace horizontal_selector_tests {

struct HorizontalSelectorFixture {
    FakeDeviceView display;
    FakeInput input;
    FakeUtilityService utility;
    HorizontalSelector selector{display, input, utility};
};

void queueKey(FakeInput& input, char key) {
    input.blockingChars.push_back(key);
}

void queueReadKey(FakeInput& input, char key) {
    input.nonBlockingChars.push_back(key);
}

void test_select_returns_initial_index_on_ok_and_draws_prompt() {
    HorizontalSelectorFixture fixture;
    queueKey(fixture.input, KEY_OK);

    const std::vector<std::string> options = {"Serial", "WiFi"};
    const int selected = fixture.selector.select("Terminal", options, "Pick one", "Use arrows");

    TEST_ASSERT_EQUAL_INT(0, selected);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.display.topBarTitles.size());
    TEST_ASSERT_EQUAL_STRING("Terminal", fixture.display.topBarTitles[0].c_str());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.display.horizontalSelectionCalls.size());
    TEST_ASSERT_EQUAL_UINT16(0, fixture.display.horizontalSelectionCalls[0].selectedIndex);
    TEST_ASSERT_EQUAL_STRING("Serial", fixture.display.horizontalSelectionCalls[0].options[0].c_str());
    TEST_ASSERT_EQUAL_STRING("WiFi", fixture.display.horizontalSelectionCalls[0].options[1].c_str());
    TEST_ASSERT_EQUAL_STRING("Pick one", fixture.display.horizontalSelectionCalls[0].description1.c_str());
    TEST_ASSERT_EQUAL_STRING("Use arrows", fixture.display.horizontalSelectionCalls[0].description2.c_str());
}

void test_select_moves_right_and_wraps_to_first_option() {
    HorizontalSelectorFixture fixture;
    queueKey(fixture.input, KEY_ARROW_RIGHT);
    queueKey(fixture.input, KEY_ARROW_RIGHT);
    queueKey(fixture.input, KEY_ARROW_RIGHT);
    queueKey(fixture.input, KEY_OK);

    const std::vector<std::string> options = {"A", "B", "C"};
    const int selected = fixture.selector.select("Menu", options);

    TEST_ASSERT_EQUAL_INT(0, selected);
    TEST_ASSERT_EQUAL_UINT32(4, fixture.display.horizontalSelectionCalls.size());
    TEST_ASSERT_EQUAL_UINT16(0, fixture.display.horizontalSelectionCalls[0].selectedIndex);
    TEST_ASSERT_EQUAL_UINT16(1, fixture.display.horizontalSelectionCalls[1].selectedIndex);
    TEST_ASSERT_EQUAL_UINT16(2, fixture.display.horizontalSelectionCalls[2].selectedIndex);
    TEST_ASSERT_EQUAL_UINT16(0, fixture.display.horizontalSelectionCalls[3].selectedIndex);
}

void test_select_moves_left_and_wraps_to_last_option() {
    HorizontalSelectorFixture fixture;
    queueKey(fixture.input, KEY_ARROW_LEFT);
    queueKey(fixture.input, KEY_OK);

    const std::vector<std::string> options = {"A", "B", "C"};
    const int selected = fixture.selector.select("Menu", options);

    TEST_ASSERT_EQUAL_INT(2, selected);
    TEST_ASSERT_EQUAL_UINT32(2, fixture.display.horizontalSelectionCalls.size());
    TEST_ASSERT_EQUAL_UINT16(0, fixture.display.horizontalSelectionCalls[0].selectedIndex);
    TEST_ASSERT_EQUAL_UINT16(2, fixture.display.horizontalSelectionCalls[1].selectedIndex);
}

void test_select_ignores_unknown_key_without_redrawing_same_index() {
    HorizontalSelectorFixture fixture;
    queueKey(fixture.input, 'x');
    queueKey(fixture.input, KEY_OK);

    const std::vector<std::string> options = {"A", "B"};
    const int selected = fixture.selector.select("Menu", options);

    TEST_ASSERT_EQUAL_INT(0, selected);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.display.horizontalSelectionCalls.size());
    TEST_ASSERT_EQUAL_UINT16(0, fixture.display.horizontalSelectionCalls[0].selectedIndex);
}

void test_select_headless_defaults_to_serial_after_timeout() {
    HorizontalSelectorFixture fixture;
    fixture.utility.advanceTimeOnSleep = true;
    for (int i = 0; i < 300; ++i) {
        queueReadKey(fixture.input, KEY_NONE);
    }

    const int selected = fixture.selector.selectHeadless();

    TEST_ASSERT_EQUAL_INT(2, selected);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.display.horizontalSelectionCalls.size());
    TEST_ASSERT_EQUAL_UINT16(2, fixture.display.horizontalSelectionCalls[0].selectedIndex);
    TEST_ASSERT_EQUAL_STRING("ESP32 BIT PIRATE", fixture.display.topBarTitles[0].c_str());
}

void test_select_headless_short_press_selects_wifi_client() {
    HorizontalSelectorFixture fixture;
    queueReadKey(fixture.input, KEY_OK);
    queueReadKey(fixture.input, KEY_NONE);

    const int selected = fixture.selector.selectHeadless();

    TEST_ASSERT_EQUAL_INT(0, selected);
    TEST_ASSERT_EQUAL_UINT32(2, fixture.display.horizontalSelectionCalls.size());
    TEST_ASSERT_EQUAL_UINT16(0, fixture.display.horizontalSelectionCalls[1].selectedIndex);
    TEST_ASSERT_EQUAL_STRING("Terminal selected",
                             fixture.display.horizontalSelectionCalls[1].description1.c_str());
    TEST_ASSERT_EQUAL_STRING("Connecting to WiFi...",
                             fixture.display.horizontalSelectionCalls[1].description2.c_str());
    TEST_ASSERT_EQUAL_UINT32(250, fixture.utility.lastSleepMs);
}

void test_select_headless_long_press_selects_hotspot() {
    HorizontalSelectorFixture fixture;
    fixture.utility.advanceTimeOnSleep = true;
    queueReadKey(fixture.input, KEY_OK);
    for (int i = 0; i < 90; ++i) {
        queueReadKey(fixture.input, KEY_OK);
    }

    const int selected = fixture.selector.selectHeadless();

    TEST_ASSERT_EQUAL_INT(1, selected);
    TEST_ASSERT_EQUAL_UINT32(2, fixture.display.horizontalSelectionCalls.size());
    TEST_ASSERT_EQUAL_UINT16(1, fixture.display.horizontalSelectionCalls[1].selectedIndex);
    TEST_ASSERT_EQUAL_STRING("Terminal selected",
                             fixture.display.horizontalSelectionCalls[1].description1.c_str());
    TEST_ASSERT_EQUAL_STRING("Starting hotspot...",
                             fixture.display.horizontalSelectionCalls[1].description2.c_str());
    TEST_ASSERT_EQUAL_UINT32(250, fixture.utility.lastSleepMs);
}

}  // namespace horizontal_selector_tests

void runHorizontalSelectorTests() {
    using namespace horizontal_selector_tests;
    RUN_TEST(test_select_returns_initial_index_on_ok_and_draws_prompt);
    RUN_TEST(test_select_moves_right_and_wraps_to_first_option);
    RUN_TEST(test_select_moves_left_and_wraps_to_last_option);
    RUN_TEST(test_select_ignores_unknown_key_without_redrawing_same_index);
    RUN_TEST(test_select_headless_defaults_to_serial_after_timeout);
    RUN_TEST(test_select_headless_short_press_selects_wifi_client);
    RUN_TEST(test_select_headless_long_press_selects_hotspot);
}
