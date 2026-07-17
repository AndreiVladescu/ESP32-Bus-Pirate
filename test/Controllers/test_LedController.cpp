#include <unity.h>

#include "Controllers/LedController.h"
#include "../Inputs/FakeInput.h"
#include "../Services/FakeLedService.h"
#include "../Services/FakeUtilityService.h"
#include "../Views/FakeTerminalView.h"

namespace led_controller_tests {

struct LedControllerFixture {
    FakeTerminalView view;
    FakeInput input;
    FakeUtilityService utility;
    FakeLedService ledService;
    ArgTransformer transformer;
    UserInputManager userInput{view, input, transformer};
    HelpShell helpShell{view, input, userInput};
    LedController controller{
        view,
        input,
        utility,
        ledService,
        transformer,
        userInput,
        helpShell
    };

    LedControllerFixture() {
        auto& state = GlobalState::getInstance();
        state.setCurrentMode(ModeEnum::LED);
        state.setLedDataPin(1);
        state.setLedClockPin(2);
        state.setLedLength(1);
        state.setLedProtocol("ws2812");
        state.setLedBrightness(128);
    }
};

void assertColor(const CRGB& color, uint8_t red, uint8_t green, uint8_t blue) {
    TEST_ASSERT_EQUAL_UINT8(red, color.r);
    TEST_ASSERT_EQUAL_UINT8(green, color.g);
    TEST_ASSERT_EQUAL_UINT8(blue, color.b);
}

void test_led_fill_parses_rgb_triplet() {
    LedControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("fill", "10", "20 30"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.ledService.fillCalls.size());
    assertColor(fixture.ledService.fillCalls[0], 10, 20, 30);
}

void test_led_fill_delegates_html_color_parsing() {
    LedControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("fill", "#AABBCC"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.ledService.htmlColorInputs.size());
    TEST_ASSERT_EQUAL_STRING("#AABBCC", fixture.ledService.htmlColorInputs[0].c_str());
    assertColor(fixture.ledService.fillCalls[0], 4, 5, 6);
}

void test_led_fill_delegates_named_color_parsing() {
    LedControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("fill", "blue"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.ledService.stringColorInputs.size());
    TEST_ASSERT_EQUAL_STRING("blue", fixture.ledService.stringColorInputs[0].c_str());
    assertColor(fixture.ledService.fillCalls[0], 1, 2, 3);
}

void test_led_set_requires_index_and_color() {
    LedControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("set", "0"));

    TEST_ASSERT_TRUE(fixture.ledService.setCalls.empty());
    TEST_ASSERT_TRUE(fixture.view.contains("Usage: set <index>"));
}

void test_led_set_rejects_invalid_index() {
    LedControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("set", "first", "red"));

    TEST_ASSERT_TRUE(fixture.ledService.setCalls.empty());
    TEST_ASSERT_TRUE(fixture.view.contains("Invalid index format"));
}

void test_led_set_can_correct_index_one_for_single_led_strip() {
    LedControllerFixture fixture;
    fixture.input.queueLine("");

    fixture.controller.handleCommand(TerminalCommand("set", "1", "red"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.ledService.setCalls.size());
    TEST_ASSERT_EQUAL_UINT16(0, fixture.ledService.setCalls[0].index);
}

void test_led_set_can_cancel_single_led_index_correction() {
    LedControllerFixture fixture;
    fixture.input.queueLine("n");

    fixture.controller.handleCommand(TerminalCommand("set", "1", "red"));

    TEST_ASSERT_TRUE(fixture.ledService.setCalls.empty());
}

void test_led_reset_without_index_resets_entire_strip() {
    LedControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("reset"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.ledService.resetCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("Reset all LEDs"));
}

void test_led_reset_index_sets_led_to_black() {
    LedControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("reset", "3"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.ledService.setCalls.size());
    TEST_ASSERT_EQUAL_UINT16(3, fixture.ledService.setCalls[0].index);
    assertColor(fixture.ledService.setCalls[0].color, 0, 0, 0);
}

void test_led_reset_rejects_invalid_index() {
    LedControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("reset", "bad"));

    TEST_ASSERT_TRUE(fixture.ledService.setCalls.empty());
    TEST_ASSERT_TRUE(fixture.view.contains("Invalid syntax"));
}

void test_led_animation_runs_until_enter() {
    LedControllerFixture fixture;
    fixture.input.queueReadChar('\0');
    fixture.input.queueReadChar('\n');

    fixture.controller.handleCommand(TerminalCommand("rainbow"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.ledService.animationCalls.size());
    TEST_ASSERT_EQUAL_STRING("rainbow", fixture.ledService.animationCalls[0].c_str());
    TEST_ASSERT_TRUE(fixture.view.contains("Animation stopped"));
}

void test_led_animation_rejects_type_not_supported_by_service() {
    LedControllerFixture fixture;
    fixture.ledService.animations.clear();

    fixture.controller.handleCommand(TerminalCommand("blink"));

    TEST_ASSERT_TRUE(fixture.ledService.animationCalls.empty());
    TEST_ASSERT_TRUE(fixture.view.contains("Unknown animation type: blink"));
}

void test_led_config_applies_length_brightness_and_current_protocol() {
    LedControllerFixture fixture;
    fixture.input.queueLine("10");
    fixture.input.queueLine("200");

    fixture.controller.handleCommand(TerminalCommand("config"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.ledService.configureCalls);
    TEST_ASSERT_EQUAL_UINT8(1, fixture.ledService.lastConfiguration.dataPin);
    TEST_ASSERT_EQUAL_UINT8(2, fixture.ledService.lastConfiguration.clockPin);
    TEST_ASSERT_EQUAL_UINT16(10, fixture.ledService.lastConfiguration.length);
    TEST_ASSERT_EQUAL_UINT8(200, fixture.ledService.lastConfiguration.brightness);
    TEST_ASSERT_EQUAL_STRING("ws2812", fixture.ledService.lastConfiguration.protocol.c_str());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.ledService.resetCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("external 5V supply"));
}

void test_led_ensure_configured_prompts_once_then_reconfigures() {
    LedControllerFixture fixture;
    fixture.input.queueLine("");
    fixture.input.queueLine("");

    fixture.controller.ensureConfigured();
    fixture.controller.ensureConfigured();

    TEST_ASSERT_EQUAL_UINT32(2, fixture.ledService.configureCalls);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.ledService.resetCalls);
}

void test_led_set_protocol_selects_combined_protocol_list() {
    LedControllerFixture fixture;
    fixture.input.queueLine("3");
    fixture.input.queueLine("");
    fixture.input.queueLine("");

    fixture.controller.handleCommand(TerminalCommand("setprotocol"));

    TEST_ASSERT_EQUAL_STRING("apa102", GlobalState::getInstance().getLedProtocol().c_str());
    TEST_ASSERT_EQUAL_STRING("apa102", fixture.ledService.lastConfiguration.protocol.c_str());
    TEST_ASSERT_TRUE(fixture.view.contains("LED protocol changed to apa102"));
}

void test_led_instruction_reports_not_implemented() {
    LedControllerFixture fixture;

    fixture.controller.handleInstruction({ByteCode(ByteCodeEnum::Write, 1)});

    TEST_ASSERT_TRUE(fixture.view.contains("LED instructions not implemented"));
}

void test_led_unknown_command_displays_help() {
    LedControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("unknown"));

    TEST_ASSERT_TRUE(fixture.view.contains("Unknown command. Available LED commands"));
    TEST_ASSERT_TRUE(fixture.view.contains("fill <color>"));
}

}  // namespace led_controller_tests

void runLedControllerTests() {
    using namespace led_controller_tests;
    RUN_TEST(test_led_fill_parses_rgb_triplet);
    RUN_TEST(test_led_fill_delegates_html_color_parsing);
    RUN_TEST(test_led_fill_delegates_named_color_parsing);
    RUN_TEST(test_led_set_requires_index_and_color);
    RUN_TEST(test_led_set_rejects_invalid_index);
    RUN_TEST(test_led_set_can_correct_index_one_for_single_led_strip);
    RUN_TEST(test_led_set_can_cancel_single_led_index_correction);
    RUN_TEST(test_led_reset_without_index_resets_entire_strip);
    RUN_TEST(test_led_reset_index_sets_led_to_black);
    RUN_TEST(test_led_reset_rejects_invalid_index);
    RUN_TEST(test_led_animation_runs_until_enter);
    RUN_TEST(test_led_animation_rejects_type_not_supported_by_service);
    RUN_TEST(test_led_config_applies_length_brightness_and_current_protocol);
    RUN_TEST(test_led_ensure_configured_prompts_once_then_reconfigures);
    RUN_TEST(test_led_set_protocol_selects_combined_protocol_list);
    RUN_TEST(test_led_instruction_reports_not_implemented);
    RUN_TEST(test_led_unknown_command_displays_help);
}
