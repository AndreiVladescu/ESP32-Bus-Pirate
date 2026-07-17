#include <unity.h>

#include "Controllers/FmController.h"
#include "../Inputs/FakeInput.h"
#include "../Services/FakeFmService.h"
#include "../Services/FakeUtilityService.h"
#include "../Shells/FakeShell.h"
#include "../Views/FakeDeviceView.h"
#include "../Views/FakeTerminalView.h"

namespace fm_controller_tests {

struct FmControllerFixture {
    FakeTerminalView view;
    FakeInput input;
    FakeDeviceView device;
    FakeUtilityService utility;
    FakeFmService service;
    ArgTransformer argTransformer;
    UserInputManager userInput{view, input, argTransformer};
    HelpShell helpShell{view, input, userInput};
    FakeShell broadcastShell;
    FmController controller{
        view,
        input,
        device,
        utility,
        service,
        argTransformer,
        userInput,
        helpShell,
        broadcastShell
    };

    FmControllerFixture() {
        auto& state = GlobalState::getInstance();
        state.setCurrentMode(ModeEnum::FM);
        state.setTerminalMode(TerminalTypeEnum::Standalone);
        state.setTwoWireIoPin(1);
        state.setTwoWireClkPin(2);
        state.setTwoWireRstPin(3);
        state.setI2cFrequency(100000);
    }

    void configureSuccessfully(uint8_t sda = 4,
                               uint8_t scl = 5,
                               uint8_t reset = 6,
                               uint32_t frequency = 400000) {
        input.queueLine(std::to_string(sda));
        input.queueLine(std::to_string(scl));
        input.queueLine(std::to_string(reset));
        input.queueLine(std::to_string(frequency));
        controller.handleCommand(TerminalCommand("config"));
        view.output.clear();
        view.printCalls.clear();
        view.printlnCalls.clear();
    }
};

void test_config_applies_i2c_reset_pins_and_frequency() {
    FmControllerFixture fixture;
    fixture.input.queueLine("7");
    fixture.input.queueLine("8");
    fixture.input.queueLine("9");
    fixture.input.queueLine("400000");

    fixture.controller.handleCommand(TerminalCommand("config"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.resetPins.size());
    TEST_ASSERT_EQUAL_UINT8(9, fixture.service.resetPins[0]);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.configurations.size());
    TEST_ASSERT_EQUAL_INT8(9, fixture.service.configurations[0].resetPin);
    TEST_ASSERT_EQUAL_INT8(7, fixture.service.configurations[0].sdaPin);
    TEST_ASSERT_EQUAL_INT8(8, fixture.service.configurations[0].sclPin);
    TEST_ASSERT_EQUAL_UINT32(400000, fixture.service.configurations[0].i2cFreqHz);
    TEST_ASSERT_EQUAL_UINT8(7, GlobalState::getInstance().getTwoWireIoPin());
    TEST_ASSERT_EQUAL_UINT8(8, GlobalState::getInstance().getTwoWireClkPin());
    TEST_ASSERT_EQUAL_UINT8(9, GlobalState::getInstance().getTwoWireRstPin());
    TEST_ASSERT_TRUE(fixture.view.contains("configured successfully"));
}

void test_config_reports_service_configuration_failure_without_begin() {
    FmControllerFixture fixture;
    fixture.service.configureResult = false;
    fixture.input.queueLine("7");
    fixture.input.queueLine("8");
    fixture.input.queueLine("9");
    fixture.input.queueLine("400000");

    fixture.controller.handleCommand(TerminalCommand("config"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.resetPins.size());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.configurations.size());
    TEST_ASSERT_EQUAL_UINT32(0, fixture.service.beginCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("configure failed"));
}

void test_config_reports_missing_chip_after_successful_bus_configuration() {
    FmControllerFixture fixture;
    fixture.service.beginResult = false;
    fixture.input.queueLine("7");
    fixture.input.queueLine("8");
    fixture.input.queueLine("9");
    fixture.input.queueLine("400000");

    fixture.controller.handleCommand(TerminalCommand("config"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.beginCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("SI4713 not detected"));
}

void test_ensure_configured_reapplies_saved_state_without_prompting_again() {
    FmControllerFixture fixture;
    fixture.input.queueLine("7");
    fixture.input.queueLine("8");
    fixture.input.queueLine("9");
    fixture.input.queueLine("400000");

    fixture.controller.ensureConfigured();
    GlobalState::getInstance().setTwoWireIoPin(10);
    GlobalState::getInstance().setTwoWireClkPin(11);
    GlobalState::getInstance().setTwoWireRstPin(12);
    GlobalState::getInstance().setI2cFrequency(1000000);
    fixture.controller.ensureConfigured();

    TEST_ASSERT_EQUAL_UINT32(2, fixture.service.configurations.size());
    TEST_ASSERT_EQUAL_INT8(9, fixture.service.configurations[0].resetPin);
    TEST_ASSERT_EQUAL_INT8(7, fixture.service.configurations[0].sdaPin);
    TEST_ASSERT_EQUAL_INT8(8, fixture.service.configurations[0].sclPin);
    TEST_ASSERT_EQUAL_INT8(12, fixture.service.configurations[1].resetPin);
    TEST_ASSERT_EQUAL_INT8(10, fixture.service.configurations[1].sdaPin);
    TEST_ASSERT_EQUAL_INT8(11, fixture.service.configurations[1].sclPin);
    TEST_ASSERT_EQUAL_UINT32(1000000, fixture.service.configurations[1].i2cFreqHz);
}

void test_sweep_measures_requested_frequency_and_prints_noise() {
    FmControllerFixture fixture;
    fixture.configureSuccessfully();
    fixture.input.queueLine("87");
    fixture.input.queueLine("88");
    fixture.input.queueLine("1");
    fixture.input.queueLine("1");
    fixture.input.queueLine("0");
    fixture.input.queueReadChar(KEY_NONE);
    fixture.input.queueReadChar(KEY_NONE);
    fixture.input.queueReadChar('\n');
    fixture.service.noiseResults.push_back(55);

    fixture.controller.handleCommand(TerminalCommand("sweep"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.measuredFrequencies.size());
    TEST_ASSERT_EQUAL_UINT16(8700, fixture.service.measuredFrequencies[0]);
    TEST_ASSERT_TRUE(fixture.view.contains("noise= 55"));
    TEST_ASSERT_TRUE(fixture.view.contains("FM Sweep: Stopped by user"));
}

void test_trace_reports_measure_failure_after_setting_screen_title() {
    FmControllerFixture fixture;
    fixture.configureSuccessfully();
    fixture.utility.queueNowMs(0);
    fixture.utility.queueNowMs(10);
    fixture.input.queueReadChar(KEY_NONE);
    fixture.service.measureResult = false;

    fixture.controller.handleCommand(TerminalCommand("trace", "99.5"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.device.topBarTitles.size());
    TEST_ASSERT_EQUAL_STRING("99.50 MHz", fixture.device.topBarTitles[0].c_str());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.measuredFrequencies.size());
    TEST_ASSERT_EQUAL_UINT16(9950, fixture.service.measuredFrequencies[0]);
    TEST_ASSERT_TRUE(fixture.view.contains("FM Trace: measure failed"));
}

void test_waterfall_draws_noise_level_for_measured_frequency() {
    FmControllerFixture fixture;
    fixture.configureSuccessfully();
    fixture.input.queueLine("87");
    fixture.input.queueLine("88");
    fixture.input.queueLine("1");
    fixture.input.queueLine("1");
    fixture.input.queueReadChar(KEY_NONE);
    fixture.input.queueReadChar(KEY_NONE);
    fixture.input.queueReadChar('\n');
    fixture.service.noiseResults.push_back(80);

    fixture.controller.handleCommand(TerminalCommand("waterfall"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.measuredFrequencies.size());
    TEST_ASSERT_EQUAL_UINT16(8700, fixture.service.measuredFrequencies[0]);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.device.waterfallCalls.size());
    TEST_ASSERT_EQUAL_STRING("MHz", fixture.device.waterfallCalls[0].unit.c_str());
    TEST_ASSERT_EQUAL_INT(100, fixture.device.waterfallCalls[0].level);
    TEST_ASSERT_TRUE(fixture.view.contains("FM Waterfall: Stopped by user"));
}

void test_broadcast_delegates_to_shell_when_chip_is_present() {
    FmControllerFixture fixture;
    fixture.configureSuccessfully();

    fixture.controller.handleCommand(TerminalCommand("broadcast"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.broadcastShell.runCalls);
}

void test_broadcast_reports_absent_chip_without_running_shell() {
    FmControllerFixture fixture;
    fixture.configureSuccessfully();
    fixture.service.beginResult = false;

    fixture.controller.handleCommand(TerminalCommand("broadcast"));

    TEST_ASSERT_EQUAL_UINT32(0, fixture.broadcastShell.runCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("SI4713 not detected"));
}

void test_reset_uses_saved_reset_pin() {
    FmControllerFixture fixture;
    GlobalState::getInstance().setTwoWireRstPin(18);

    fixture.controller.handleCommand(TerminalCommand("reset"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.resetPins.size());
    TEST_ASSERT_EQUAL_UINT8(18, fixture.service.resetPins[0]);
    TEST_ASSERT_TRUE(fixture.view.contains("Resetting SI4713"));
}

void test_unknown_command_displays_help() {
    FmControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("wat"));

    TEST_ASSERT_TRUE(fixture.view.contains("Unknown command. Available FM commands"));
}

}  // namespace fm_controller_tests

void runFmControllerTests() {
    using namespace fm_controller_tests;
    RUN_TEST(test_config_applies_i2c_reset_pins_and_frequency);
    RUN_TEST(test_config_reports_service_configuration_failure_without_begin);
    RUN_TEST(test_config_reports_missing_chip_after_successful_bus_configuration);
    RUN_TEST(test_ensure_configured_reapplies_saved_state_without_prompting_again);
    RUN_TEST(test_sweep_measures_requested_frequency_and_prints_noise);
    RUN_TEST(test_trace_reports_measure_failure_after_setting_screen_title);
    RUN_TEST(test_waterfall_draws_noise_level_for_measured_frequency);
    RUN_TEST(test_broadcast_delegates_to_shell_when_chip_is_present);
    RUN_TEST(test_broadcast_reports_absent_chip_without_running_shell);
    RUN_TEST(test_reset_uses_saved_reset_pin);
    RUN_TEST(test_unknown_command_displays_help);
}
