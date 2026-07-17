#include <unity.h>

#include "Controllers/CanController.h"
#include "../Inputs/FakeInput.h"
#include "../Services/FakeCanService.h"
#include "../Services/FakeUtilityService.h"
#include "../Views/FakeTerminalView.h"

namespace can_controller_tests {

struct CanControllerFixture {
    FakeTerminalView view;
    FakeInput input;
    FakeUtilityService utility;
    FakeCanService canService;
    ArgTransformer transformer;
    UserInputManager userInput{view, input, transformer};
    HelpShell helpShell{view, input, userInput};
    CanController controller{
        view,
        input,
        userInput,
        utility,
        canService,
        transformer,
        helpShell
    };

    CanControllerFixture() {
        auto& state = GlobalState::getInstance();
        state.setCurrentMode(ModeEnum::CAN_);
        state.setCanCspin(1);
        state.setCanSckPin(0);
        state.setCanSiPin(2);
        state.setCanSoPin(3);
        state.setCanKbps(120);
    }
};

void test_status_displays_service_status() {
    CanControllerFixture fixture;
    fixture.canService.status = "CAN OK: TX=0 RX=0";

    fixture.controller.handleCommand(TerminalCommand("status"));

    TEST_ASSERT_TRUE(fixture.view.contains("CAN Status:"));
    TEST_ASSERT_TRUE(fixture.view.contains("CAN OK: TX=0 RX=0"));
}

void test_send_parses_standard_id_and_payload() {
    CanControllerFixture fixture;
    fixture.input.queueLine("01 02 0A FF");

    fixture.controller.handleCommand(TerminalCommand("send", "0x321"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.canService.sentFrames.size());
    TEST_ASSERT_EQUAL_HEX32(0x321, fixture.canService.sentFrames[0].id);
    const uint8_t expected[] = {0x01, 0x02, 0x0A, 0xFF};
    TEST_ASSERT_EQUAL_UINT32(4, fixture.canService.sentFrames[0].data.size());
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, fixture.canService.sentFrames[0].data.data(), 4);
    TEST_ASSERT_TRUE(fixture.view.contains("CAN Send: ✅ Frame sent to 0x321"));
}

void test_send_reports_service_failure() {
    CanControllerFixture fixture;
    fixture.canService.sendResult = false;
    fixture.input.queueLine("AA");

    fixture.controller.handleCommand(TerminalCommand("send", "123"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.canService.sentFrames.size());
    TEST_ASSERT_TRUE(fixture.view.contains("CAN Send: ❌ Failed to send frame to 0x07B"));
}

void test_send_rejects_id_above_standard_11_bit_range() {
    CanControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("send", "0x800"));

    TEST_ASSERT_TRUE(fixture.canService.sentFrames.empty());
    TEST_ASSERT_TRUE(fixture.view.contains("Only 11-bit standard IDs are supported"));
}

void test_send_rejects_id_that_would_overflow_16_bits() {
    CanControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("send", "0x10000"));

    TEST_ASSERT_TRUE(fixture.canService.sentFrames.empty());
    TEST_ASSERT_TRUE(fixture.view.contains("Only 11-bit standard IDs are supported"));
}

void test_send_rejects_payload_larger_than_classic_can_frame() {
    CanControllerFixture fixture;
    fixture.input.queueLine("00 01 02 03 04 05 06 07 08");

    fixture.controller.handleCommand(TerminalCommand("send", "0x123"));

    TEST_ASSERT_TRUE(fixture.canService.sentFrames.empty());
    TEST_ASSERT_TRUE(fixture.view.contains("at most 8 data bytes"));
}

void test_receive_sets_filter_flushes_and_displays_frame() {
    CanControllerFixture fixture;
    fixture.canService.receivedFrames.push_back("ID:123 DLC:2 DATA:AA BB");
    fixture.input.queueReadChar('\n');

    fixture.controller.handleCommand(TerminalCommand("receive", "0x123"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.canService.setFilterCalls);
    TEST_ASSERT_EQUAL_HEX32(0x123, fixture.canService.lastFilter);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.canService.flushCalls);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.canService.resetCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("ID:123 DLC:2 DATA:AA BB"));
    TEST_ASSERT_TRUE(fixture.view.contains("Can Receive: Stopped by user."));
}

void test_receive_rejects_id_above_standard_11_bit_range() {
    CanControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("receive", "2048"));

    TEST_ASSERT_EQUAL_UINT32(0, fixture.canService.setFilterCalls);
    TEST_ASSERT_EQUAL_UINT32(0, fixture.canService.flushCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("Only 11-bit standard IDs are supported"));
}

void test_receive_rejects_id_that_would_overflow_16_bits() {
    CanControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("receive", "65536"));

    TEST_ASSERT_EQUAL_UINT32(0, fixture.canService.setFilterCalls);
    TEST_ASSERT_EQUAL_UINT32(0, fixture.canService.flushCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("Only 11-bit standard IDs are supported"));
}

void test_receive_reapplies_filter_after_inactivity_reset() {
    CanControllerFixture fixture;
    fixture.utility.queueNowMs(0);
    fixture.utility.queueNowMs(3001);
    fixture.utility.queueNowMs(3001);
    fixture.utility.queueNowMs(3001);
    fixture.input.queueReadChar('\0');
    fixture.input.queueReadChar('\n');

    fixture.controller.handleCommand(TerminalCommand("receive", "0x456"));

    TEST_ASSERT_EQUAL_UINT32(2, fixture.canService.setFilterCalls);
    TEST_ASSERT_EQUAL_HEX32(0x456, fixture.canService.lastFilter);
    TEST_ASSERT_EQUAL_UINT32(2, fixture.canService.resetCalls);
}

void test_sniff_resets_controller_and_displays_received_frame() {
    CanControllerFixture fixture;
    fixture.canService.receivedFrames.push_back("ID:456 DLC:1 DATA:7F");
    fixture.input.queueReadChar('\r');

    fixture.controller.handleCommand(TerminalCommand("sniff"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.canService.resetCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("ID:456 DLC:1 DATA:7F"));
    TEST_ASSERT_TRUE(fixture.view.contains("Can Sniff: Stopped by user."));
}

void test_sniff_resets_controller_after_three_seconds_without_frame() {
    CanControllerFixture fixture;
    fixture.utility.queueNowMs(0);
    fixture.utility.queueNowMs(3001);
    fixture.utility.queueNowMs(3001);
    fixture.input.queueReadChar('\n');

    fixture.controller.handleCommand(TerminalCommand("sniff"));

    TEST_ASSERT_EQUAL_UINT32(2, fixture.canService.resetCalls);
}

void test_config_applies_pins_and_closest_supported_bitrate() {
    CanControllerFixture fixture;
    fixture.canService.supportedBitrate = 250;
    fixture.input.queueLine("5");
    fixture.input.queueLine("6");
    fixture.input.queueLine("7");
    fixture.input.queueLine("333");

    fixture.controller.handleCommand(TerminalCommand("config"));

    TEST_ASSERT_EQUAL_UINT32(333, fixture.canService.requestedBitrate);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.canService.configureCalls);
    TEST_ASSERT_EQUAL_UINT8(1, fixture.canService.lastConfiguration.csPin);
    TEST_ASSERT_EQUAL_UINT8(5, fixture.canService.lastConfiguration.sckPin);
    TEST_ASSERT_EQUAL_UINT8(7, fixture.canService.lastConfiguration.misoPin);
    TEST_ASSERT_EQUAL_UINT8(6, fixture.canService.lastConfiguration.mosiPin);
    TEST_ASSERT_EQUAL_UINT32(250, fixture.canService.lastConfiguration.bitrateKbps);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.canService.probeCalls);
    TEST_ASSERT_EQUAL_UINT32(250, GlobalState::getInstance().getCanKbps());
    TEST_ASSERT_TRUE(fixture.view.contains("Using 250 kbps instead"));
    TEST_ASSERT_TRUE(fixture.view.contains("MCP2515 CAN configured"));
}

void test_config_reports_probe_failure() {
    CanControllerFixture fixture;
    fixture.canService.probeResult = false;
    fixture.input.queueLine("");
    fixture.input.queueLine("");
    fixture.input.queueLine("");
    fixture.input.queueLine("");

    fixture.controller.handleCommand(TerminalCommand("config"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.canService.probeCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("MCP2515 CAN configuration failed"));
    TEST_ASSERT_FALSE(fixture.view.contains("MCP2515 CAN configured."));
}

void test_config_rejects_spi_pin_equal_to_fixed_cs_pin() {
    CanControllerFixture fixture;
    fixture.input.queueLine("1");
    fixture.input.queueLine("5");
    fixture.input.queueLine("6");
    fixture.input.queueLine("7");
    fixture.input.queueLine("125");

    fixture.controller.handleCommand(TerminalCommand("config"));

    TEST_ASSERT_EQUAL_UINT8(5, fixture.canService.lastConfiguration.sckPin);
    TEST_ASSERT_TRUE(fixture.view.contains("reserved/protected"));
}

void test_ensure_configured_prompts_once_then_reapplies_configuration() {
    CanControllerFixture fixture;
    fixture.canService.supportedBitrate = 125;
    fixture.input.queueLine("");
    fixture.input.queueLine("");
    fixture.input.queueLine("");
    fixture.input.queueLine("");

    fixture.controller.ensureConfigured();
    fixture.controller.ensureConfigured();

    TEST_ASSERT_EQUAL_UINT32(2, fixture.canService.configureCalls);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.canService.probeCalls);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.input.waitPressCalls);
}

void test_ensure_configured_retries_after_probe_failure() {
    CanControllerFixture fixture;
    fixture.canService.probeResult = false;
    for (int i = 0; i < 8; ++i) fixture.input.queueLine("");

    fixture.controller.ensureConfigured();
    fixture.controller.ensureConfigured();

    TEST_ASSERT_EQUAL_UINT32(2, fixture.canService.configureCalls);
    TEST_ASSERT_EQUAL_UINT32(2, fixture.canService.probeCalls);
    TEST_ASSERT_EQUAL_UINT32(2, fixture.input.waitPressCalls);
}

void test_unknown_command_displays_can_help() {
    CanControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("unknown"));

    TEST_ASSERT_TRUE(fixture.view.contains("Unknown command. Available CAN commands"));
    TEST_ASSERT_TRUE(fixture.view.contains("CAN (MCP2515)"));
    TEST_ASSERT_TRUE(fixture.view.contains("send [id]"));
}

}  // namespace can_controller_tests

void runCanControllerTests() {
    using namespace can_controller_tests;
    RUN_TEST(test_status_displays_service_status);
    RUN_TEST(test_send_parses_standard_id_and_payload);
    RUN_TEST(test_send_reports_service_failure);
    RUN_TEST(test_send_rejects_id_above_standard_11_bit_range);
    RUN_TEST(test_send_rejects_id_that_would_overflow_16_bits);
    RUN_TEST(test_send_rejects_payload_larger_than_classic_can_frame);
    RUN_TEST(test_receive_sets_filter_flushes_and_displays_frame);
    RUN_TEST(test_receive_rejects_id_above_standard_11_bit_range);
    RUN_TEST(test_receive_rejects_id_that_would_overflow_16_bits);
    RUN_TEST(test_receive_reapplies_filter_after_inactivity_reset);
    RUN_TEST(test_sniff_resets_controller_and_displays_received_frame);
    RUN_TEST(test_sniff_resets_controller_after_three_seconds_without_frame);
    RUN_TEST(test_config_applies_pins_and_closest_supported_bitrate);
    RUN_TEST(test_config_reports_probe_failure);
    RUN_TEST(test_config_rejects_spi_pin_equal_to_fixed_cs_pin);
    RUN_TEST(test_ensure_configured_prompts_once_then_reapplies_configuration);
    RUN_TEST(test_ensure_configured_retries_after_probe_failure);
    RUN_TEST(test_unknown_command_displays_can_help);
}
