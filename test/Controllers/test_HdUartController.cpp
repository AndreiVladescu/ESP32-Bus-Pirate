#include <unity.h>

#include "Controllers/HdUartController.h"
#include "../Inputs/FakeInput.h"
#include "../Services/FakeHdUartService.h"
#include "../Services/FakeUartService.h"
#include "../Views/FakeTerminalView.h"

namespace hd_uart_controller_tests {

struct HdUartControllerFixture {
    FakeTerminalView view;
    FakeInput terminalInput;
    FakeInput deviceInput;
    FakeHdUartService hdUartService;
    FakeUartService uartService;
    ArgTransformer transformer;
    UserInputManager userInput{view, terminalInput, transformer};
    HelpShell helpShell{view, terminalInput, userInput};
    HdUartController controller{
        view,
        terminalInput,
        deviceInput,
        hdUartService,
        uartService,
        transformer,
        userInput,
        helpShell
    };

    HdUartControllerFixture() {
        auto& state = GlobalState::getInstance();
        state.setCurrentMode(ModeEnum::HDUART);
        state.setHdUartPin(1);
        state.setHdUartBaudRate(9600);
        state.setHdUartDataBits(8);
        state.setHdUartParity("N");
        state.setHdUartStopBits(1);
        state.setHdUartInverted(false);
    }
};

void queueDefaultConfiguration(HdUartControllerFixture& fixture) {
    for (int i = 0; i < 6; ++i) fixture.terminalInput.queueLine("");
}

void test_instruction_delegates_bytecodes_and_displays_result() {
    HdUartControllerFixture fixture;
    fixture.hdUartService.byteCodeResult = "AA BB";
    const std::vector<ByteCode> bytecodes = {
        ByteCode(ByteCodeEnum::Write, 0x42),
        ByteCode(ByteCodeEnum::Read, 2)
    };

    fixture.controller.handleInstruction(bytecodes);

    TEST_ASSERT_EQUAL_UINT32(2, fixture.hdUartService.lastBytecodes.size());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ByteCodeEnum::Write),
                          static_cast<int>(fixture.hdUartService.lastBytecodes[0].getCommand()));
    TEST_ASSERT_EQUAL_HEX32(0x42, fixture.hdUartService.lastBytecodes[0].getData());
    TEST_ASSERT_TRUE(fixture.view.contains("HDUART Read:"));
    TEST_ASSERT_TRUE(fixture.view.contains("AA BB"));
}

void test_instruction_reports_no_data_for_empty_service_result() {
    HdUartControllerFixture fixture;

    fixture.controller.handleInstruction({ByteCode(ByteCodeEnum::Read, 1)});

    TEST_ASSERT_EQUAL_UINT32(1, fixture.hdUartService.lastBytecodes.size());
    TEST_ASSERT_TRUE(fixture.view.contains("HDUART Read: No data"));
}

void test_config_applies_every_selected_serial_parameter() {
    HdUartControllerFixture fixture;
    fixture.terminalInput.queueLine("4");
    fixture.terminalInput.queueLine("19200");
    fixture.terminalInput.queueLine("7");
    fixture.terminalInput.queueLine("e");
    fixture.terminalInput.queueLine("2");
    fixture.terminalInput.queueLine("y");

    fixture.controller.handleCommand(TerminalCommand("config"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.hdUartService.configurations.size());
    const auto& config = fixture.hdUartService.configurations[0];
    TEST_ASSERT_EQUAL_UINT32(19200, config.baud);
    TEST_ASSERT_EQUAL_UINT8(7, config.dataBits);
    TEST_ASSERT_EQUAL_CHAR('E', config.parity);
    TEST_ASSERT_EQUAL_UINT8(2, config.stopBits);
    TEST_ASSERT_EQUAL_UINT8(4, config.ioPin);
    TEST_ASSERT_TRUE(config.inverted);
    TEST_ASSERT_EQUAL_UINT8(4, GlobalState::getInstance().getHdUartPin());
    TEST_ASSERT_EQUAL_UINT32(19200, GlobalState::getInstance().getHdUartBaudRate());
}

void test_ensure_configured_releases_full_duplex_uart_first() {
    HdUartControllerFixture fixture;
    fixture.uartService.installed = true;
    queueDefaultConfiguration(fixture);

    fixture.controller.ensureConfigured();

    TEST_ASSERT_EQUAL_UINT32(1, fixture.uartService.endCalls);
    TEST_ASSERT_FALSE(fixture.uartService.installed);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.hdUartService.configurations.size());
}

void test_ensure_configured_prompts_once_then_reapplies_saved_state() {
    HdUartControllerFixture fixture;
    queueDefaultConfiguration(fixture);

    fixture.controller.ensureConfigured();
    fixture.controller.ensureConfigured();

    TEST_ASSERT_EQUAL_UINT32(2, fixture.hdUartService.configurations.size());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.hdUartService.endCalls);
    const auto& reapplied = fixture.hdUartService.configurations[1];
    TEST_ASSERT_EQUAL_UINT32(9600, reapplied.baud);
    TEST_ASSERT_EQUAL_UINT8(8, reapplied.dataBits);
    TEST_ASSERT_EQUAL_CHAR('N', reapplied.parity);
    TEST_ASSERT_EQUAL_UINT8(1, reapplied.stopBits);
    TEST_ASSERT_EQUAL_UINT8(1, reapplied.ioPin);
    TEST_ASSERT_FALSE(reapplied.inverted);
}

void test_bridge_forwards_terminal_character_and_stops_on_device_input() {
    HdUartControllerFixture fixture;
    fixture.terminalInput.queueReadChar('x');
    fixture.deviceInput.queueReadChar('\n');

    fixture.controller.handleCommand(TerminalCommand("bridge"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.hdUartService.byteWrites.size());
    TEST_ASSERT_EQUAL_UINT8('x', fixture.hdUartService.byteWrites[0]);
    TEST_ASSERT_TRUE(fixture.view.contains("HDUART Bridge: Stopped by user"));
}

void test_bridge_displays_device_data() {
    HdUartControllerFixture fixture;
    fixture.hdUartService.queueRx("OK");
    fixture.terminalInput.queueReadChar(KEY_NONE);
    fixture.deviceInput.queueReadChar('\n');

    fixture.controller.handleCommand(TerminalCommand("bridge"));

    TEST_ASSERT_TRUE(fixture.view.contains("OK"));
    TEST_ASSERT_TRUE(fixture.hdUartService.byteWrites.empty());
}

void test_bridge_filters_the_local_echo_of_transmitted_data() {
    HdUartControllerFixture fixture;
    fixture.hdUartService.echoWrites = true;
    fixture.terminalInput.queueReadChar('Q');
    fixture.deviceInput.queueReadChar(KEY_NONE);
    fixture.deviceInput.queueReadChar('\n');

    fixture.controller.handleCommand(TerminalCommand("bridge"));

    bool printedEcho = false;
    for (const auto& call : fixture.view.printCalls) {
        if (call == "Q") printedEcho = true;
    }
    TEST_ASSERT_FALSE(printedEcho);
    TEST_ASSERT_EQUAL_UINT8('Q', fixture.hdUartService.byteWrites[0]);
}

void test_unknown_command_displays_hduart_help() {
    HdUartControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("unknown"));

    TEST_ASSERT_TRUE(fixture.view.contains("Unknown command. Available HDUART commands"));
    TEST_ASSERT_TRUE(fixture.view.contains("Half-duplex I/O"));
}

}  // namespace hd_uart_controller_tests

void runHdUartControllerTests() {
    using namespace hd_uart_controller_tests;
    RUN_TEST(test_instruction_delegates_bytecodes_and_displays_result);
    RUN_TEST(test_instruction_reports_no_data_for_empty_service_result);
    RUN_TEST(test_config_applies_every_selected_serial_parameter);
    RUN_TEST(test_ensure_configured_releases_full_duplex_uart_first);
    RUN_TEST(test_ensure_configured_prompts_once_then_reapplies_saved_state);
    RUN_TEST(test_bridge_forwards_terminal_character_and_stops_on_device_input);
    RUN_TEST(test_bridge_displays_device_data);
    RUN_TEST(test_bridge_filters_the_local_echo_of_transmitted_data);
    RUN_TEST(test_unknown_command_displays_hduart_help);
}
