#include <unity.h>

#include "Controllers/ExpanderController.h"
#include "../Inputs/FakeInput.h"
#include "../Services/FakeUartService.h"
#include "../Services/FakeUtilityService.h"
#include "../Views/FakeTerminalView.h"

namespace expander_controller_tests {

struct ExpanderControllerFixture {
    FakeTerminalView view;
    FakeInput input;
    FakeUtilityService utility;
    FakeUartService uartService;
    ArgTransformer transformer;
    UserInputManager userInput{view, input, transformer};
    HelpShell helpShell{view, input, userInput};
    ExpanderController controller{
        view,
        input,
        utility,
        uartService,
        transformer,
        userInput,
        helpShell
    };

    ExpanderControllerFixture() {
        auto& state = GlobalState::getInstance();
        state.setCurrentMode(ModeEnum::EXPANDER);
        state.setUartRxPin(1);
        state.setUartTxPin(2);
    }
};

void queuePins(ExpanderControllerFixture& fixture, const std::string& rx = "4",
               const std::string& tx = "5") {
    fixture.input.queueLine(rx);
    fixture.input.queueLine(tx);
}

void queueExit(ExpanderControllerFixture& fixture) {
    for (const char value : std::string("exit\n")) fixture.input.queueReadChar(value);
}

void test_successful_handshake_uses_fixed_115200_8n1_configuration() {
    ExpanderControllerFixture fixture;
    fixture.uartService.uartConfigResult = 0x12345678;
    fixture.uartService.responseAfterHandshake = "ready [[BP-HANDSHAKE-OK]]\n";
    queuePins(fixture);
    queueExit(fixture);

    fixture.controller.handleCommand(TerminalCommand("connect"));

    TEST_ASSERT_EQUAL_UINT8(8, fixture.uartService.lastDataBits);
    TEST_ASSERT_EQUAL_CHAR('N', fixture.uartService.lastParity);
    TEST_ASSERT_EQUAL_UINT8(1, fixture.uartService.lastStopBits);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.uartService.configurations.size());
    const auto& config = fixture.uartService.configurations[0];
    TEST_ASSERT_EQUAL_UINT32(115200, config.baud);
    TEST_ASSERT_EQUAL_HEX32(0x12345678, config.config);
    TEST_ASSERT_EQUAL_UINT8(4, config.rx);
    TEST_ASSERT_EQUAL_UINT8(5, config.tx);
    TEST_ASSERT_FALSE(config.inverted);
    TEST_ASSERT_TRUE(fixture.uartService.wrote("handshake\n"));
    TEST_ASSERT_TRUE(fixture.view.contains("Expander handshake OK"));
}

void test_handshake_sends_eight_crlf_pairs_before_probe() {
    ExpanderControllerFixture fixture;
    fixture.uartService.responseAfterHandshake = "[[BP-HANDSHAKE-OK]]";
    queuePins(fixture);
    queueExit(fixture);

    fixture.controller.handleCommand(TerminalCommand("connect"));

    uint32_t carriageReturns = 0;
    uint32_t lineFeedsBeforeHandshake = 0;
    for (const auto& write : fixture.uartService.writes) {
        if (write == "handshake\n") break;
        if (write == "\r") ++carriageReturns;
        if (write == "\n") ++lineFeedsBeforeHandshake;
    }
    TEST_ASSERT_EQUAL_UINT32(8, carriageReturns);
    TEST_ASSERT_EQUAL_UINT32(8, lineFeedsBeforeHandshake);
}

void test_bridge_forwards_session_data_and_exit_closes_it() {
    ExpanderControllerFixture fixture;
    fixture.uartService.responseAfterHandshake = "[[BP-HANDSHAKE-OK]]";
    queuePins(fixture);
    fixture.input.queueReadChar('a');
    fixture.input.queueReadChar('b');
    fixture.input.queueReadChar('\n');
    queueExit(fixture);

    fixture.controller.handleCommand(TerminalCommand("connect"));

    TEST_ASSERT_TRUE(fixture.uartService.wrote("a"));
    TEST_ASSERT_TRUE(fixture.uartService.wrote("b"));
    TEST_ASSERT_TRUE(fixture.uartService.wrote("e"));
    TEST_ASSERT_EQUAL_UINT32(1, fixture.uartService.flushCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("Expander session closed"));
}

void test_stale_uart_input_is_discarded_before_handshake() {
    ExpanderControllerFixture fixture;
    fixture.uartService.queueRx("stale prompt");
    fixture.uartService.responseAfterHandshake = "[[BP-HANDSHAKE-OK]]";
    queuePins(fixture);
    queueExit(fixture);

    fixture.controller.handleCommand(TerminalCommand("connect"));

    TEST_ASSERT_FALSE(fixture.view.contains("stale prompt"));
    TEST_ASSERT_TRUE(fixture.view.contains("Expander handshake OK"));
}

void test_failed_handshake_returns_to_high_impedance_mode() {
    ExpanderControllerFixture fixture;
    fixture.utility.advanceTimeOnSleep = true;
    queuePins(fixture);

    fixture.controller.handleCommand(TerminalCommand("connect"));

    TEST_ASSERT_EQUAL_INT(static_cast<int>(ModeEnum::HIZ),
                          static_cast<int>(GlobalState::getInstance().getCurrentMode()));
    TEST_ASSERT_TRUE(fixture.view.contains("Expander handshake failed"));
    TEST_ASSERT_TRUE(fixture.view.contains("swap RX/TX"));
    TEST_ASSERT_EQUAL_UINT32(0, fixture.uartService.flushCalls);
}

void test_failed_handshake_can_be_retried_with_new_pins() {
    ExpanderControllerFixture fixture;
    fixture.utility.advanceTimeOnSleep = true;
    queuePins(fixture, "4", "5");
    queuePins(fixture, "6", "7");

    fixture.controller.handleCommand(TerminalCommand("connect"));
    fixture.controller.handleCommand(TerminalCommand("connect"));

    TEST_ASSERT_EQUAL_UINT32(2, fixture.uartService.configurations.size());
    TEST_ASSERT_EQUAL_UINT8(4, fixture.uartService.configurations[0].rx);
    TEST_ASSERT_EQUAL_UINT8(5, fixture.uartService.configurations[0].tx);
    TEST_ASSERT_EQUAL_UINT8(6, fixture.uartService.configurations[1].rx);
    TEST_ASSERT_EQUAL_UINT8(7, fixture.uartService.configurations[1].tx);
}

void test_closed_session_can_be_started_again_with_new_pins() {
    ExpanderControllerFixture fixture;
    fixture.uartService.responseAfterHandshake = "[[BP-HANDSHAKE-OK]]";
    queuePins(fixture, "4", "5");
    queueExit(fixture);
    fixture.controller.handleCommand(TerminalCommand("connect"));

    queuePins(fixture, "6", "7");
    queueExit(fixture);
    fixture.controller.handleCommand(TerminalCommand("connect"));

    TEST_ASSERT_EQUAL_UINT32(2, fixture.uartService.configurations.size());
    TEST_ASSERT_EQUAL_UINT8(4, fixture.uartService.configurations[0].rx);
    TEST_ASSERT_EQUAL_UINT8(5, fixture.uartService.configurations[0].tx);
    TEST_ASSERT_EQUAL_UINT8(6, fixture.uartService.configurations[1].rx);
    TEST_ASSERT_EQUAL_UINT8(7, fixture.uartService.configurations[1].tx);
    TEST_ASSERT_TRUE(fixture.view.contains("Expander session closed"));
}

void test_bridge_forwards_escape_sequence_as_three_bytes() {
    ExpanderControllerFixture fixture;
    fixture.uartService.responseAfterHandshake = "[[BP-HANDSHAKE-OK]]";
    queuePins(fixture);
    fixture.input.queueReadChar('\x1B');
    fixture.input.queueReadChar('[');
    fixture.input.queueReadChar('A');
    queueExit(fixture);

    fixture.controller.handleCommand(TerminalCommand("connect"));

    TEST_ASSERT_TRUE(fixture.uartService.wrote("\x1B"));
    TEST_ASSERT_TRUE(fixture.uartService.wrote("["));
    TEST_ASSERT_TRUE(fixture.uartService.wrote("A"));
}

void test_bridge_backspace_updates_exit_detection_buffer() {
    ExpanderControllerFixture fixture;
    fixture.uartService.responseAfterHandshake = "[[BP-HANDSHAKE-OK]]";
    queuePins(fixture);
    for (const char value : std::string("exx\bit\n")) {
        fixture.input.queueReadChar(value);
    }

    fixture.controller.handleCommand(TerminalCommand("connect"));

    TEST_ASSERT_TRUE(fixture.uartService.wrote("\b"));
    TEST_ASSERT_EQUAL_UINT32(1, fixture.uartService.flushCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("Expander session closed"));
}

}  // namespace expander_controller_tests

void runExpanderControllerTests() {
    using namespace expander_controller_tests;
    RUN_TEST(test_successful_handshake_uses_fixed_115200_8n1_configuration);
    RUN_TEST(test_handshake_sends_eight_crlf_pairs_before_probe);
    RUN_TEST(test_bridge_forwards_session_data_and_exit_closes_it);
    RUN_TEST(test_stale_uart_input_is_discarded_before_handshake);
    RUN_TEST(test_failed_handshake_returns_to_high_impedance_mode);
    RUN_TEST(test_failed_handshake_can_be_retried_with_new_pins);
    RUN_TEST(test_closed_session_can_be_started_again_with_new_pins);
    RUN_TEST(test_bridge_forwards_escape_sequence_as_three_bytes);
    RUN_TEST(test_bridge_backspace_updates_exit_detection_buffer);
}
