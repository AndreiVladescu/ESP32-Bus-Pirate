#include <unity.h>

#include "Controllers/DioController.h"
#include "../Inputs/FakeInput.h"
#include "../Services/FakePinService.h"
#include "../Services/FakeUtilityService.h"
#include "../Views/FakeDeviceView.h"
#include "../Views/FakeTerminalView.h"

namespace dio_controller_tests {

struct DioControllerFixture {
    FakeTerminalView view;
    FakeInput input;
    FakeDeviceView deviceView;
    FakeUtilityService utility;
    FakePinService pinService;
    ArgTransformer transformer;
    UserInputManager userInput{view, input, transformer};
    HelpShell helpShell{view, input, userInput};
    DioController controller{
        view,
        input,
        deviceView,
        utility,
        pinService,
        transformer,
        helpShell,
        userInput
    };

    DioControllerFixture() {
        GlobalState::getInstance().setCurrentMode(ModeEnum::DIO);
    }
};

void test_dio_read_requires_numeric_gpio() {
    DioControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("read", "gpio"));

    TEST_ASSERT_TRUE(fixture.pinService.readCalls.empty());
    TEST_ASSERT_TRUE(fixture.view.contains("Usage: read <gpio>"));
}

void test_dio_read_displays_high_state() {
    DioControllerFixture fixture;
    fixture.pinService.defaultRead = true;

    fixture.controller.handleCommand(TerminalCommand("read", "6"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.pinService.readCalls.size());
    TEST_ASSERT_EQUAL_UINT8(6, fixture.pinService.readCalls[0]);
    TEST_ASSERT_TRUE(fixture.view.contains("GPIO 6 = 1 (HIGH)"));
}

void test_dio_rejects_gpio_above_esp32s3_range() {
    DioControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("read", "49"));

    TEST_ASSERT_TRUE(fixture.pinService.readCalls.empty());
    TEST_ASSERT_TRUE(fixture.view.contains("out of range (0-48)"));
}

void test_dio_set_input_delegates_to_pin_service() {
    DioControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("set", "4", "in"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.pinService.inputCalls.size());
    TEST_ASSERT_EQUAL_UINT8(4, fixture.pinService.inputCalls[0]);
    TEST_ASSERT_TRUE(fixture.view.contains("set to INPUT"));
}

void test_dio_set_output_delegates_to_pin_service() {
    DioControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("set", "4", "out"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.pinService.outputCalls.size());
    TEST_ASSERT_TRUE(fixture.pinService.highCalls.empty());
    TEST_ASSERT_TRUE(fixture.pinService.lowCalls.empty());
    TEST_ASSERT_TRUE(fixture.view.contains("set to OUTPUT"));
}

void test_dio_set_high_configures_output_before_level() {
    DioControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("set", "8", "1"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.pinService.outputCalls.size());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.pinService.highCalls.size());
    TEST_ASSERT_EQUAL_UINT8(8, fixture.pinService.highCalls[0]);
    TEST_ASSERT_TRUE(fixture.view.contains("Pin 8 set to HIGH"));
}

void test_dio_set_low_configures_output_before_level() {
    DioControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("set", "8", "low"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.pinService.outputCalls.size());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.pinService.lowCalls.size());
    TEST_ASSERT_EQUAL_UINT8(8, fixture.pinService.lowCalls[0]);
    TEST_ASSERT_TRUE(fixture.view.contains("Pin 8 set to LOW"));
}

void test_dio_set_rejects_unknown_mode() {
    DioControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("set", "8", "banana"));

    TEST_ASSERT_TRUE(fixture.pinService.outputCalls.empty());
    TEST_ASSERT_TRUE(fixture.view.contains("Unknown command. Use I, O, H"));
}

void test_dio_pullup_configures_input_pullup() {
    DioControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("pullup", "9"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.pinService.inputPullupCalls.size());
    TEST_ASSERT_EQUAL_UINT8(9, fixture.pinService.inputPullupCalls[0]);
    TEST_ASSERT_TRUE(fixture.view.contains("DIO Pullup: Set on GPIO 9"));
}

void test_dio_pulldown_configures_input_pulldown() {
    DioControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("pulldown", "10"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.pinService.inputPulldownCalls.size());
    TEST_ASSERT_EQUAL_UINT8(10, fixture.pinService.inputPulldownCalls[0]);
    TEST_ASSERT_TRUE(fixture.view.contains("DIO Pulldown: Set on GPIO 10"));
}

void test_dio_pins_lists_pull_and_output_state() {
    DioControllerFixture fixture;
    fixture.pinService.defaultRead = true;
    fixture.pinService.pulls[20] = IPinService::PULL_UP;
    fixture.pinService.inputModes[20] = true;
    fixture.pinService.pulls[21] = IPinService::PULL_DOWN;
    fixture.pinService.inputModes[21] = false;

    fixture.controller.handleCommand(TerminalCommand("pins"));

    TEST_ASSERT_TRUE(fixture.view.contains("GPIO |  PULL  |    STATE"));
    TEST_ASSERT_TRUE(fixture.view.contains("  20 | PULLUP | INPUT (HIGH)"));
    TEST_ASSERT_TRUE(fixture.view.contains("  21 | PDOWN  | OUTPUT"));
}

void test_dio_pwm_passes_explicit_frequency_and_duty() {
    DioControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("pwm", "11", "2000 75"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.pinService.pwmCalls);
    TEST_ASSERT_EQUAL_UINT8(11, fixture.pinService.lastPwmPin);
    TEST_ASSERT_EQUAL_UINT32(2000, fixture.pinService.lastPwmFrequency);
    TEST_ASSERT_EQUAL_UINT8(75, fixture.pinService.lastPwmDuty);
    TEST_ASSERT_TRUE(fixture.view.contains("2000Hz, 75% duty"));
}

void test_dio_pwm_reports_service_failure() {
    DioControllerFixture fixture;
    fixture.pinService.pwmResult = false;

    fixture.controller.handleCommand(TerminalCommand("pwm", "11", "2000 75"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.pinService.pwmCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("Cannot set 11"));
}

void test_dio_measure_counts_edges() {
    DioControllerFixture fixture;
    fixture.pinService.readValues.push_back(false);
    fixture.pinService.readValues.push_back(true);
    fixture.pinService.readValues.push_back(false);
    fixture.utility.queueNowMs(0);
    fixture.utility.queueNowMs(0);
    fixture.utility.queueNowMs(1);
    fixture.utility.queueNowMs(2);

    fixture.controller.handleCommand(TerminalCommand("measure", "16", "2"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.pinService.inputCalls.size());
    TEST_ASSERT_EQUAL_UINT8(16, fixture.pinService.inputCalls[0]);
    TEST_ASSERT_TRUE(fixture.view.contains("Rising edges:     1"));
    TEST_ASSERT_TRUE(fixture.view.contains("Falling edges:    1"));
    TEST_ASSERT_TRUE(fixture.view.contains("Approx. frequency: 500.00 Hz"));
}

void test_dio_toggle_switches_pin_once_then_stops() {
    DioControllerFixture fixture;
    fixture.utility.queueNowMs(0);
    fixture.utility.queueNowMs(0);
    fixture.utility.queueNowMs(10);
    fixture.utility.queueNowMs(20);
    fixture.input.queueReadChar('\n');

    fixture.controller.handleCommand(TerminalCommand("toggle", "17", "10"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.pinService.outputCalls.size());
    TEST_ASSERT_EQUAL_UINT8(17, fixture.pinService.outputCalls[0]);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.pinService.highCalls.size());
    TEST_ASSERT_EQUAL_UINT8(17, fixture.pinService.highCalls[0]);
    TEST_ASSERT_TRUE(fixture.view.contains("DIO Toggle: Stopped."));
}

void test_dio_jam_random_toggles_until_enter() {
    DioControllerFixture fixture;
    fixture.utility.randomValue = 3;
    fixture.utility.queueNowMs(0);
    fixture.utility.queueNowMs(0);
    fixture.utility.queueNowMs(20);
    fixture.input.queueReadChar('\n');

    fixture.controller.handleCommand(TerminalCommand("jam", "18", "5 10"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.pinService.outputCalls.size());
    TEST_ASSERT_EQUAL_UINT8(18, fixture.pinService.outputCalls[0]);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.pinService.highCalls.size());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.utility.sleepUsCalls);
    TEST_ASSERT_EQUAL_UINT32(8, fixture.utility.lastSleepUs);
    TEST_ASSERT_TRUE(fixture.view.contains("DIO Jam: Stopped by user."));
}

void test_dio_sniff_reports_edge_delta() {
    DioControllerFixture fixture;
    fixture.pinService.readValues.push_back(true);
    fixture.pinService.readValues.push_back(false);
    fixture.pinService.readValues.push_back(true);
    fixture.utility.queueNowMs(0);
    fixture.utility.queueNowMs(0);
    fixture.utility.queueNowMs(1);
    fixture.utility.queueNowMs(20);
    fixture.input.queueReadChar('\n');

    fixture.controller.handleCommand(TerminalCommand("sniff", "19"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.pinService.inputCalls.size());
    TEST_ASSERT_EQUAL_UINT8(19, fixture.pinService.inputCalls[0]);
    TEST_ASSERT_TRUE(fixture.view.contains("GPIO 19: LOW  -> HIGH | delta=1000us"));
    TEST_ASSERT_TRUE(fixture.view.contains("DIO Sniff: Stopped."));
}

void test_dio_reset_detaches_signal_and_restores_input() {
    DioControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("reset", "12"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.pinService.detachCalls.size());
    TEST_ASSERT_EQUAL_UINT8(12, fixture.pinService.detachCalls[0]);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.pinService.inputCalls.size());
    TEST_ASSERT_EQUAL_UINT8(12, fixture.pinService.inputCalls[0]);
}

void test_dio_servo_passes_pin_and_angle() {
    DioControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("servo", "13", "90"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.pinService.servoCalls);
    TEST_ASSERT_EQUAL_UINT8(13, fixture.pinService.lastServoPin);
    TEST_ASSERT_EQUAL_UINT8(90, fixture.pinService.lastServoAngle);
    TEST_ASSERT_TRUE(fixture.view.contains("angle 90"));
}

void test_dio_pulse_inverts_low_pin_then_restores_it() {
    DioControllerFixture fixture;
    fixture.pinService.defaultRead = false;

    fixture.controller.handleCommand(TerminalCommand("pulse", "14", "25"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.pinService.outputCalls.size());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.pinService.highCalls.size());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.pinService.lowCalls.size());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.utility.sleepUsCalls);
    TEST_ASSERT_EQUAL_UINT32(25, fixture.utility.lastSleepUs);
    TEST_ASSERT_TRUE(fixture.view.contains("pulsed HIGH for 25 us"));
}

void test_dio_pulse_inverts_high_pin_then_restores_it() {
    DioControllerFixture fixture;
    fixture.pinService.defaultRead = true;

    fixture.controller.handleCommand(TerminalCommand("pulse", "15", "40"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.pinService.lowCalls.size());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.pinService.highCalls.size());
    TEST_ASSERT_TRUE(fixture.view.contains("pulsed LOW for 40 us"));
}

void test_dio_build_pull_lines_sorts_pins_and_limits_display_to_four() {
    DioControllerFixture fixture;
    fixture.pinService.configuredPullPins = {9, 2, 7, 1, 5};
    fixture.pinService.pulls[1] = IPinService::PULL_UP;
    fixture.pinService.pulls[2] = IPinService::PULL_DOWN;
    fixture.pinService.pulls[5] = IPinService::PULL_UP;
    fixture.pinService.pulls[7] = IPinService::PULL_DOWN;
    fixture.pinService.pulls[9] = IPinService::PULL_UP;

    const auto lines = fixture.controller.buildPullConfigLines();

    TEST_ASSERT_EQUAL_UINT32(4, lines.size());
    TEST_ASSERT_EQUAL_STRING("GPIO 1 PULLUP", lines[0].c_str());
    TEST_ASSERT_EQUAL_STRING("GPIO 2 PULLDOWN", lines[1].c_str());
    TEST_ASSERT_EQUAL_STRING("GPIO 5 PULLUP", lines[2].c_str());
    TEST_ASSERT_EQUAL_STRING("GPIO 7 PULLDOWN ...", lines[3].c_str());
}

void test_dio_unknown_command_displays_help() {
    DioControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("unknown"));

    TEST_ASSERT_TRUE(fixture.view.contains("Unknown command. Available DIO commands"));
    TEST_ASSERT_TRUE(fixture.view.contains("\nDIO\n"));
    TEST_ASSERT_TRUE(fixture.view.contains("pwm <gpio>"));
}

}  // namespace dio_controller_tests

void runDioControllerTests() {
    using namespace dio_controller_tests;
    RUN_TEST(test_dio_read_requires_numeric_gpio);
    RUN_TEST(test_dio_read_displays_high_state);
    RUN_TEST(test_dio_rejects_gpio_above_esp32s3_range);
    RUN_TEST(test_dio_set_input_delegates_to_pin_service);
    RUN_TEST(test_dio_set_output_delegates_to_pin_service);
    RUN_TEST(test_dio_set_high_configures_output_before_level);
    RUN_TEST(test_dio_set_low_configures_output_before_level);
    RUN_TEST(test_dio_set_rejects_unknown_mode);
    RUN_TEST(test_dio_pullup_configures_input_pullup);
    RUN_TEST(test_dio_pulldown_configures_input_pulldown);
    RUN_TEST(test_dio_pins_lists_pull_and_output_state);
    RUN_TEST(test_dio_pwm_passes_explicit_frequency_and_duty);
    RUN_TEST(test_dio_pwm_reports_service_failure);
    RUN_TEST(test_dio_measure_counts_edges);
    RUN_TEST(test_dio_toggle_switches_pin_once_then_stops);
    RUN_TEST(test_dio_jam_random_toggles_until_enter);
    RUN_TEST(test_dio_sniff_reports_edge_delta);
    RUN_TEST(test_dio_reset_detaches_signal_and_restores_input);
    RUN_TEST(test_dio_servo_passes_pin_and_angle);
    RUN_TEST(test_dio_pulse_inverts_low_pin_then_restores_it);
    RUN_TEST(test_dio_pulse_inverts_high_pin_then_restores_it);
    RUN_TEST(test_dio_build_pull_lines_sorts_pins_and_limits_display_to_four);
    RUN_TEST(test_dio_unknown_command_displays_help);
}
