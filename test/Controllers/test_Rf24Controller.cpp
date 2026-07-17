#include <unity.h>

#include <string>
#include <vector>

#include "Controllers/Rf24Controller.h"
#include "../Inputs/FakeInput.h"
#include "../Services/FakePinService.h"
#include "../Services/FakeRf24Service.h"
#include "../Services/FakeUtilityService.h"
#include "../Views/FakeDeviceView.h"
#include "../Views/FakeTerminalView.h"

namespace rf24_controller_tests {

struct Rf24ControllerFixture {
    FakeTerminalView view;
    FakeInput input;
    FakeDeviceView device;
    FakeUtilityService utility;
    FakeRf24Service rf24Service;
    FakePinService pinService;
    ArgTransformer transformer;
    UserInputManager userInput{view, input, transformer};
    HelpShell helpShell{view, input, userInput};
    Rf24Controller controller{
        view,
        input,
        device,
        utility,
        rf24Service,
        pinService,
        transformer,
        userInput,
        helpShell
    };

    Rf24ControllerFixture() {
        auto& state = GlobalState::getInstance();
        state.setCurrentMode(ModeEnum::RF24_);
        state.setRf24CsnPin(1);
        state.setRf24CePin(3);
        state.setRf24SckPin(5);
        state.setRf24MisoPin(7);
        state.setRf24MosiPin(9);
    }
};

void queueDefaultConfiguration(Rf24ControllerFixture& fixture) {
    fixture.input.queueLine("");
    fixture.input.queueLine("");
    fixture.input.queueLine("");
    fixture.input.queueLine("");
    fixture.input.queueLine("");
}

void queueDefaultTx(Rf24ControllerFixture& fixture, const std::string& payload = "hi") {
    fixture.input.queueLine("");
    fixture.input.queueLine("");
    fixture.input.queueLine("AABBCCDDEE");
    fixture.input.queueLine("");
    fixture.input.queueLine("");
    fixture.input.queueLine("");
    fixture.input.queueLine("");
    fixture.input.queueLine(payload);
}

void test_config_updates_state_and_reports_success() {
    Rf24ControllerFixture fixture;
    fixture.input.queueLine("10");
    fixture.input.queueLine("11");
    fixture.input.queueLine("12");
    fixture.input.queueLine("13");
    fixture.input.queueLine("14");

    fixture.controller.handleCommand(TerminalCommand("config"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.rf24Service.configureCalls.size());
    const auto& call = fixture.rf24Service.configureCalls[0];
    TEST_ASSERT_EQUAL_UINT8(10, call.csnPin);
    TEST_ASSERT_EQUAL_UINT8(14, call.cePin);
    TEST_ASSERT_EQUAL_UINT8(11, call.sckPin);
    TEST_ASSERT_EQUAL_UINT8(12, call.misoPin);
    TEST_ASSERT_EQUAL_UINT8(13, call.mosiPin);
    TEST_ASSERT_EQUAL_UINT8(10, GlobalState::getInstance().getRf24CsnPin());
    TEST_ASSERT_EQUAL_UINT8(14, GlobalState::getInstance().getRf24CePin());
    TEST_ASSERT_TRUE(fixture.view.contains("NRF24 detected and configured"));
}

void test_config_reports_missing_chip() {
    Rf24ControllerFixture fixture;
    fixture.rf24Service.configureResult = false;
    queueDefaultConfiguration(fixture);

    fixture.controller.handleCommand(TerminalCommand("config"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.rf24Service.configureCalls.size());
    TEST_ASSERT_TRUE(fixture.view.contains("NRF24 not detected"));
}

void test_ensure_configured_prompts_once_then_reapplies_saved_pins() {
    Rf24ControllerFixture fixture;
    queueDefaultConfiguration(fixture);

    fixture.controller.ensureConfigured();
    fixture.controller.ensureConfigured();

    TEST_ASSERT_EQUAL_UINT32(2, fixture.rf24Service.configureCalls.size());
    const auto& reapplied = fixture.rf24Service.configureCalls[1];
    TEST_ASSERT_EQUAL_UINT8(1, reapplied.csnPin);
    TEST_ASSERT_EQUAL_UINT8(3, reapplied.cePin);
    TEST_ASSERT_EQUAL_UINT8(5, reapplied.sckPin);
    TEST_ASSERT_EQUAL_UINT8(7, reapplied.misoPin);
    TEST_ASSERT_EQUAL_UINT8(9, reapplied.mosiPin);
}

void test_setchannel_updates_service_and_prints_result() {
    Rf24ControllerFixture fixture;
    fixture.input.queueLine("42");

    fixture.controller.handleCommand(TerminalCommand("setchannel"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.rf24Service.setChannels.size());
    TEST_ASSERT_EQUAL_UINT8(42, fixture.rf24Service.setChannels[0]);
    TEST_ASSERT_TRUE(fixture.view.contains("RF24: Channel set to 42."));
}

void test_send_initializes_tx_sends_payload_and_flushes() {
    Rf24ControllerFixture fixture;
    fixture.rf24Service.channel = 12;
    queueDefaultTx(fixture, "hello");

    fixture.controller.handleCommand(TerminalCommand("send"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.rf24Service.initTxConfigs.size());
    const auto& cfg = fixture.rf24Service.initTxConfigs[0];
    TEST_ASSERT_EQUAL_UINT8(12, cfg.channel);
    TEST_ASSERT_EQUAL_UINT8(5, cfg.addrLen);
    TEST_ASSERT_TRUE(cfg.dynamicPayloads);
    TEST_ASSERT_EQUAL_STRING("AA BB CC DD EE", cfg.addrStr.c_str());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.rf24Service.sentPayloads.size());
    TEST_ASSERT_EQUAL_UINT32(5, fixture.rf24Service.sentPayloads[0].bytes.size());
    TEST_ASSERT_EQUAL_UINT8('h', fixture.rf24Service.sentPayloads[0].bytes[0]);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.rf24Service.flushTxCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("TX SENT ACK OK"));
}

void test_send_fixed_payload_pads_payload_before_send() {
    Rf24ControllerFixture fixture;
    fixture.input.queueLine("");
    fixture.input.queueLine("");
    fixture.input.queueLine("AABBCCDDEE");
    fixture.input.queueLine("");
    fixture.input.queueLine("");
    fixture.input.queueLine("n");
    fixture.input.queueLine("4");
    fixture.input.queueLine("");
    fixture.input.queueLine("hex{ AA BB }");

    fixture.controller.handleCommand(TerminalCommand("send"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.rf24Service.initTxConfigs.size());
    TEST_ASSERT_FALSE(fixture.rf24Service.initTxConfigs[0].dynamicPayloads);
    TEST_ASSERT_EQUAL_UINT8(4, fixture.rf24Service.initTxConfigs[0].fixedPayloadSize);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.rf24Service.sentPayloads.size());
    const auto& payload = fixture.rf24Service.sentPayloads[0].bytes;
    TEST_ASSERT_EQUAL_UINT32(4, payload.size());
    TEST_ASSERT_EQUAL_HEX8(0xAA, payload[0]);
    TEST_ASSERT_EQUAL_HEX8(0xBB, payload[1]);
    TEST_ASSERT_EQUAL_HEX8(0x00, payload[2]);
    TEST_ASSERT_EQUAL_HEX8(0x00, payload[3]);
}

void test_send_reports_init_failure_without_transmitting() {
    Rf24ControllerFixture fixture;
    fixture.rf24Service.initTxResult = false;
    queueDefaultTx(fixture);

    fixture.controller.handleCommand(TerminalCommand("send"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.rf24Service.initTxConfigs.size());
    TEST_ASSERT_TRUE(fixture.rf24Service.sentPayloads.empty());
    TEST_ASSERT_TRUE(fixture.view.contains("RF24 Send: init failed"));
}

void test_receive_initializes_rx_displays_frame_and_stops_cleanly() {
    Rf24ControllerFixture fixture;
    fixture.rf24Service.availablePipeValue = 2;
    fixture.rf24Service.receivedPayloads.push_back({'O', 'K'});
    fixture.input.queueLine("33");
    fixture.input.queueLine("3");
    fixture.input.queueLine("AABBCC");
    fixture.input.queueLine("2");
    fixture.input.queueLine("8");
    fixture.input.queueLine("0");
    fixture.input.queueLine("n");
    fixture.input.queueLine("4");
    fixture.input.queueReadChar(KEY_NONE);
    fixture.input.queueReadChar('\n');

    fixture.controller.handleCommand(TerminalCommand("receive"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.rf24Service.initRxConfigs.size());
    const auto& cfg = fixture.rf24Service.initRxConfigs[0];
    TEST_ASSERT_EQUAL_UINT8(33, cfg.channel);
    TEST_ASSERT_EQUAL_UINT8(2, cfg.pipe);
    TEST_ASSERT_EQUAL_UINT8(3, cfg.addrLen);
    TEST_ASSERT_EQUAL_INT(8, cfg.crcBits);
    TEST_ASSERT_EQUAL_INT(0, cfg.dataRate);
    TEST_ASSERT_FALSE(cfg.dynamicPayloads);
    TEST_ASSERT_EQUAL_UINT8(4, cfg.fixedPayloadSize);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.rf24Service.startListeningCalls);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.rf24Service.stopListeningCalls);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.rf24Service.flushRxCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("[RX pipe=2 len=2]"));
    TEST_ASSERT_TRUE(fixture.view.contains("4F 4B"));
}

void test_receive_reports_init_failure() {
    Rf24ControllerFixture fixture;
    fixture.rf24Service.initRxResult = false;
    fixture.input.queueLine("");
    fixture.input.queueLine("");
    fixture.input.queueLine("AABBCCDDEE");
    fixture.input.queueLine("");
    fixture.input.queueLine("");
    fixture.input.queueLine("");
    fixture.input.queueLine("");

    fixture.controller.handleCommand(TerminalCommand("receive"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.rf24Service.initRxConfigs.size());
    TEST_ASSERT_EQUAL_UINT32(0, fixture.rf24Service.startListeningCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("RF24 Receive: init failed"));
}

void test_scan_stops_without_activity() {
    Rf24ControllerFixture fixture;
    fixture.input.queueLine("");
    fixture.input.queueReadChar('\n');

    fixture.controller.handleCommand(TerminalCommand("scan"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.rf24Service.initRxSimpleCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("RF24: No activity detected."));
}

void test_jam_can_be_cancelled_before_touching_radio() {
    Rf24ControllerFixture fixture;
    fixture.input.queueLine("");

    fixture.controller.handleCommand(TerminalCommand("jam"));

    TEST_ASSERT_EQUAL_UINT32(0, fixture.rf24Service.powerUpCalls);
    TEST_ASSERT_EQUAL_UINT32(0, fixture.rf24Service.setPowerMaxCalls);
}

void test_waterfall_draws_one_sample_then_stops() {
    Rf24ControllerFixture fixture;
    fixture.rf24Service.rpd = true;
    fixture.input.queueLine("2");
    fixture.input.queueReadChar(KEY_NONE);
    fixture.input.queueReadChar('\n');

    fixture.controller.handleCommand(TerminalCommand("waterfall"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.rf24Service.initRxSimpleCalls);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.device.waterfallCalls.size());
    TEST_ASSERT_EQUAL_STRING("ch", fixture.device.waterfallCalls[0].unit.c_str());
    TEST_ASSERT_EQUAL_INT(0, fixture.device.waterfallCalls[0].index);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.rf24Service.flushRxCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("RF24 Waterfall: Stopped by user."));
}

void test_unknown_command_displays_rf24_help() {
    Rf24ControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("wat"));

    TEST_ASSERT_TRUE(fixture.view.contains("Unknown command. Available RF24 commands"));
    TEST_ASSERT_TRUE(fixture.view.contains("RF24"));
}

}  // namespace rf24_controller_tests

void runRf24ControllerTests() {
    using namespace rf24_controller_tests;
    RUN_TEST(test_config_updates_state_and_reports_success);
    RUN_TEST(test_config_reports_missing_chip);
    RUN_TEST(test_ensure_configured_prompts_once_then_reapplies_saved_pins);
    RUN_TEST(test_setchannel_updates_service_and_prints_result);
    RUN_TEST(test_send_initializes_tx_sends_payload_and_flushes);
    RUN_TEST(test_send_fixed_payload_pads_payload_before_send);
    RUN_TEST(test_send_reports_init_failure_without_transmitting);
    RUN_TEST(test_receive_initializes_rx_displays_frame_and_stops_cleanly);
    RUN_TEST(test_receive_reports_init_failure);
    RUN_TEST(test_scan_stops_without_activity);
    RUN_TEST(test_jam_can_be_cancelled_before_touching_radio);
    RUN_TEST(test_waterfall_draws_one_sample_then_stops);
    RUN_TEST(test_unknown_command_displays_rf24_help);
}
