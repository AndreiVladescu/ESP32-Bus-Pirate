#include <unity.h>

#include "Controllers/I2sController.h"
#include "../Inputs/FakeInput.h"
#include "../Services/FakeI2sService.h"
#include "../Services/FakeUtilityService.h"
#include "../Views/FakeTerminalView.h"

namespace i2s_controller_tests {

struct I2sControllerFixture {
    FakeTerminalView view;
    FakeInput input;
    FakeUtilityService utility;
    FakeI2sService i2sService;
    ArgTransformer transformer;
    UserInputManager userInput{view, input, transformer};
    HelpShell helpShell{view, input, userInput};
    I2sController controller{
        view,
        input,
        utility,
        i2sService,
        transformer,
        userInput,
        helpShell
    };

    I2sControllerFixture() {
        auto& state = GlobalState::getInstance();
        state.setCurrentMode(ModeEnum::I2S);
        state.setI2sBclkPin(41);
        state.setI2sLrckPin(43);
        state.setI2sDataPin(42);
        state.setI2sSampleRate(44100);
        state.setI2sBitsPerSample(16);
        state.setI2sPercentLevel(100);
    }
};

void test_i2s_play_requires_numeric_frequency() {
    I2sControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("play", "tone"));

    TEST_ASSERT_EQUAL_UINT32(0, fixture.i2sService.interruptibleToneCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("Usage: play <frequency>"));
}

void test_i2s_play_without_duration_starts_interruptible_tone() {
    I2sControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("play", "440"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.i2sService.outputConfigureCalls);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.i2sService.interruptibleToneCalls);
    TEST_ASSERT_EQUAL_UINT32(44100, fixture.i2sService.lastInterruptibleTone.sampleRate);
    TEST_ASSERT_EQUAL_UINT16(440, fixture.i2sService.lastInterruptibleTone.frequency);
    TEST_ASSERT_EQUAL_UINT32(0xFFFF, fixture.i2sService.lastInterruptibleTone.durationMs);
    TEST_ASSERT_TRUE(fixture.view.contains("I2S Play: Done"));
}

void test_i2s_play_with_duration_passes_requested_milliseconds() {
    I2sControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("play", "0x1B8", "250"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.i2sService.interruptibleToneCalls);
    TEST_ASSERT_EQUAL_UINT16(440, fixture.i2sService.lastInterruptibleTone.frequency);
    TEST_ASSERT_EQUAL_UINT32(250, fixture.i2sService.lastInterruptibleTone.durationMs);
    TEST_ASSERT_TRUE(fixture.view.contains("for 250 ms"));
}

void test_i2s_play_rejects_extra_or_invalid_duration_arguments() {
    I2sControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("play", "440", "bad value"));

    TEST_ASSERT_EQUAL_UINT32(0, fixture.i2sService.interruptibleToneCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("Usage: play <frequency>"));
}

void test_i2s_record_switches_to_input_and_reads_until_enter() {
    I2sControllerFixture fixture;
    fixture.i2sService.samples.assign(16, 100);

    fixture.controller.handleCommand(TerminalCommand("record"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.i2sService.inputConfigureCalls);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.i2sService.recordCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("I2S Record: Stopped by user"));
}

void test_i2s_mic_test_reports_read_failure() {
    I2sControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("test", "mic"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.i2sService.inputConfigureCalls);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.i2sService.recordCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("Failed to read from I2S microphone"));
}

void test_i2s_mic_test_reports_strong_signal() {
    I2sControllerFixture fixture;
    fixture.i2sService.samples = {-500, 500, -450, 450};

    fixture.controller.handleCommand(TerminalCommand("test", "mic"));

    TEST_ASSERT_TRUE(fixture.view.contains("Strong and valid signal"));
    TEST_ASSERT_TRUE(fixture.view.contains("Peak-to-peak  : 1000"));
}

void test_i2s_speaker_test_can_stop_during_first_tone() {
    I2sControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("test", "speaker"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.i2sService.tones.size());
    TEST_ASSERT_EQUAL_UINT16(262, fixture.i2sService.tones[0].frequency);
    TEST_ASSERT_EQUAL_UINT32(2, fixture.i2sService.outputConfigureCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("Speaker Test: Stopped by user"));
}

void test_i2s_test_requires_speaker_or_mic_mode() {
    I2sControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("test"));

    TEST_ASSERT_EQUAL_UINT32(0, fixture.i2sService.inputConfigureCalls);
    TEST_ASSERT_EQUAL_UINT32(0, fixture.i2sService.outputConfigureCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("Usage: test <speaker|mic>"));
}

void test_i2s_reset_reconfigures_output_mode() {
    I2sControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("reset"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.i2sService.outputConfigureCalls);
    TEST_ASSERT_EQUAL_UINT8(41, fixture.i2sService.lastOutput.bclk);
    TEST_ASSERT_EQUAL_UINT8(43, fixture.i2sService.lastOutput.lrck);
    TEST_ASSERT_EQUAL_UINT8(42, fixture.i2sService.lastOutput.data);
    TEST_ASSERT_TRUE(fixture.view.contains("TX (output) mode"));
}

void test_i2s_reports_output_initialization_failure() {
    I2sControllerFixture fixture;
    fixture.i2sService.initialized = false;

    fixture.controller.handleCommand(TerminalCommand("reset"));

    TEST_ASSERT_TRUE(fixture.view.contains("can't configure output"));
}

void test_i2s_reports_input_initialization_failure() {
    I2sControllerFixture fixture;
    fixture.i2sService.initialized = false;

    fixture.controller.handleCommand(TerminalCommand("record"));

    TEST_ASSERT_TRUE(fixture.view.contains("can't configure input"));
}

void test_i2s_config_updates_state_without_touching_hardware() {
    I2sControllerFixture fixture;
    fixture.input.queueLine("5");
    fixture.input.queueLine("6");
    fixture.input.queueLine("7");
    fixture.input.queueLine("48000");
    fixture.input.queueLine("24");
    fixture.input.queueLine("80");

    fixture.controller.handleCommand(TerminalCommand("config"));

    const auto& state = GlobalState::getInstance();
    TEST_ASSERT_EQUAL_UINT8(5, state.getI2sBclkPin());
    TEST_ASSERT_EQUAL_UINT8(6, state.getI2sLrckPin());
    TEST_ASSERT_EQUAL_UINT8(7, state.getI2sDataPin());
    TEST_ASSERT_EQUAL_UINT32(48000, state.getI2sSampleRate());
    TEST_ASSERT_EQUAL_UINT8(24, state.getI2sBitsPerSample());
    TEST_ASSERT_EQUAL_UINT8(80, state.getI2sPercentLevel());
    TEST_ASSERT_EQUAL_UINT32(0, fixture.i2sService.outputConfigureCalls);
    TEST_ASSERT_EQUAL_UINT32(0, fixture.i2sService.inputConfigureCalls);
}

void test_i2s_ensure_configured_prompts_once_then_reapplies_output() {
    I2sControllerFixture fixture;
    for (int i = 0; i < 6; ++i) fixture.input.queueLine("");

    fixture.controller.ensureConfigured();
    fixture.controller.ensureConfigured();

    TEST_ASSERT_EQUAL_UINT32(1, fixture.i2sService.outputConfigureCalls);
    TEST_ASSERT_TRUE(fixture.input.blockingChars.empty());
}

void test_i2s_unknown_command_displays_help() {
    I2sControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("unknown"));

    TEST_ASSERT_TRUE(fixture.view.contains("Unknown command. Available I2S commands"));
}

}  // namespace i2s_controller_tests

void runI2sControllerTests() {
    using namespace i2s_controller_tests;
    RUN_TEST(test_i2s_play_requires_numeric_frequency);
    RUN_TEST(test_i2s_play_without_duration_starts_interruptible_tone);
    RUN_TEST(test_i2s_play_with_duration_passes_requested_milliseconds);
    RUN_TEST(test_i2s_play_rejects_extra_or_invalid_duration_arguments);
    RUN_TEST(test_i2s_record_switches_to_input_and_reads_until_enter);
    RUN_TEST(test_i2s_mic_test_reports_read_failure);
    RUN_TEST(test_i2s_mic_test_reports_strong_signal);
    RUN_TEST(test_i2s_speaker_test_can_stop_during_first_tone);
    RUN_TEST(test_i2s_test_requires_speaker_or_mic_mode);
    RUN_TEST(test_i2s_reset_reconfigures_output_mode);
    RUN_TEST(test_i2s_reports_output_initialization_failure);
    RUN_TEST(test_i2s_reports_input_initialization_failure);
    RUN_TEST(test_i2s_config_updates_state_without_touching_hardware);
    RUN_TEST(test_i2s_ensure_configured_prompts_once_then_reapplies_output);
    RUN_TEST(test_i2s_unknown_command_displays_help);
}
