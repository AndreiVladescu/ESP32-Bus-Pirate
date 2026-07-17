#include <unity.h>

#include "Controllers/TwoWireController.h"
#include "../Inputs/FakeInput.h"
#include "../Services/FakeTwoWireService.h"
#include "../Shells/FakeShell.h"
#include "../Views/FakeTerminalView.h"

namespace two_wire_controller_tests {

struct TwoWireControllerFixture {
    FakeTerminalView view;
    FakeInput input;
    ArgTransformer transformer;
    UserInputManager userInput{view, input, transformer};
    FakeTwoWireService service;
    FakeShell smartCardShell;
    HelpShell helpShell{view, input, userInput};
    TwoWireController controller{
        view,
        input,
        userInput,
        service,
        smartCardShell,
        helpShell
    };

    TwoWireControllerFixture() {
        auto& state = GlobalState::getInstance();
        state.setCurrentMode(ModeEnum::TwoWire);
        state.setTwoWireClkPin(1);
        state.setTwoWireIoPin(2);
        state.setTwoWireRstPin(3);
    }

    void queueConfig(const std::string& clk = "", const std::string& io = "",
                     const std::string& rst = "") {
        input.queueLine(clk);
        input.queueLine(io);
        input.queueLine(rst);
    }
};

void test_config_updates_state_and_configures_service() {
    TwoWireControllerFixture fixture;
    fixture.queueConfig("7", "8", "9");

    fixture.controller.handleCommand(TerminalCommand("config"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.configurations.size());
    const auto& config = fixture.service.configurations[0];
    TEST_ASSERT_EQUAL_UINT8(7, config.clk);
    TEST_ASSERT_EQUAL_UINT8(8, config.io);
    TEST_ASSERT_EQUAL_UINT8(9, config.rst);
    TEST_ASSERT_EQUAL_UINT8(7, GlobalState::getInstance().getTwoWireClkPin());
    TEST_ASSERT_EQUAL_UINT8(8, GlobalState::getInstance().getTwoWireIoPin());
    TEST_ASSERT_EQUAL_UINT8(9, GlobalState::getInstance().getTwoWireRstPin());
    TEST_ASSERT_TRUE(fixture.view.contains("2WIRE configuration applied"));
}

void test_ensure_configured_prompts_once_then_reapplies_saved_state() {
    TwoWireControllerFixture fixture;
    fixture.queueConfig();

    fixture.controller.ensureConfigured();
    GlobalState::getInstance().setTwoWireClkPin(4);
    GlobalState::getInstance().setTwoWireIoPin(5);
    GlobalState::getInstance().setTwoWireRstPin(6);
    fixture.controller.ensureConfigured();

    TEST_ASSERT_EQUAL_UINT32(2, fixture.service.configurations.size());
    TEST_ASSERT_EQUAL_UINT8(1, fixture.service.configurations[0].clk);
    TEST_ASSERT_EQUAL_UINT8(2, fixture.service.configurations[0].io);
    TEST_ASSERT_EQUAL_UINT8(3, fixture.service.configurations[0].rst);
    TEST_ASSERT_EQUAL_UINT8(4, fixture.service.configurations[1].clk);
    TEST_ASSERT_EQUAL_UINT8(5, fixture.service.configurations[1].io);
    TEST_ASSERT_EQUAL_UINT8(6, fixture.service.configurations[1].rst);
}

void test_sniff_decodes_three_byte_command_frame_and_stops() {
    TwoWireControllerFixture fixture;
    fixture.queueConfig();
    fixture.service.sniffEvents.push_back({1, 0x00});
    fixture.service.sniffEvents.push_back({3, 0x30});
    fixture.service.sniffEvents.push_back({3, 0x12});
    fixture.service.sniffEvents.push_back({3, 0x34});
    fixture.service.sniffEvents.push_back({2, 0x00});

    fixture.controller.handleCommand(TerminalCommand("sniff"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.startSnifferCalls);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.stopSnifferCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("CMD READ_MAIN"));
    TEST_ASSERT_TRUE(fixture.view.contains("[30 12 34]"));
    TEST_ASSERT_TRUE(fixture.view.contains("2WIRE Sniffer: Stopped by user"));
}

void test_sniff_reports_start_failure() {
    TwoWireControllerFixture fixture;
    fixture.queueConfig();
    fixture.service.startSnifferResult = false;

    fixture.controller.handleCommand(TerminalCommand("sniff"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.startSnifferCalls);
    TEST_ASSERT_EQUAL_UINT32(0, fixture.service.stopSnifferCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("Failed to start sniffer"));
}

void test_sniff_formats_non_three_byte_frame_as_response_data() {
    TwoWireControllerFixture fixture;
    fixture.queueConfig();
    fixture.service.sniffEvents.push_back({1, 0x00});
    fixture.service.sniffEvents.push_back({3, 0xAB});
    fixture.service.sniffEvents.push_back({3, 0xCD});
    fixture.service.sniffEvents.push_back({2, 0x00});

    fixture.controller.handleCommand(TerminalCommand("sniff"));

    TEST_ASSERT_TRUE(fixture.view.contains("RESP data"));
    TEST_ASSERT_TRUE(fixture.view.contains("AB"));
    TEST_ASSERT_TRUE(fixture.view.contains("CD"));
}

void test_sniff_formats_unknown_three_byte_command() {
    TwoWireControllerFixture fixture;
    fixture.queueConfig();
    fixture.service.sniffEvents.push_back({1, 0x00});
    fixture.service.sniffEvents.push_back({3, 0x99});
    fixture.service.sniffEvents.push_back({3, 0x01});
    fixture.service.sniffEvents.push_back({3, 0x02});
    fixture.service.sniffEvents.push_back({2, 0x00});

    fixture.controller.handleCommand(TerminalCommand("sniff"));

    TEST_ASSERT_TRUE(fixture.view.contains("CMD UNKNOWN"));
    TEST_ASSERT_TRUE(fixture.view.contains("[99 01 02]"));
}

void test_smartcard_command_delegates_to_shell() {
    TwoWireControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("smartcard"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.smartCardShell.runCalls);
}

void test_instruction_reports_not_implemented() {
    TwoWireControllerFixture fixture;

    fixture.controller.handleInstruction({ByteCode(ByteCodeEnum::Start)});

    TEST_ASSERT_TRUE(fixture.view.contains("Instruction support for 2WIRE not yet implemented"));
}

void test_release_frees_sniffer_and_resets_configuration_state() {
    TwoWireControllerFixture fixture;
    fixture.queueConfig();
    fixture.controller.ensureConfigured();

    fixture.controller.ensureReleased();
    fixture.queueConfig("10", "11", "12");
    fixture.controller.ensureConfigured();

    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.releaseSnifferCalls);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.endCalls);
    TEST_ASSERT_EQUAL_UINT8(10, fixture.service.configurations.back().clk);
    TEST_ASSERT_EQUAL_UINT8(11, fixture.service.configurations.back().io);
    TEST_ASSERT_EQUAL_UINT8(12, fixture.service.configurations.back().rst);
}

void test_unknown_command_displays_help() {
    TwoWireControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("wat"));

    TEST_ASSERT_TRUE(fixture.view.contains("Unknown command. Available 2WIRE commands"));
    TEST_ASSERT_TRUE(fixture.view.contains("2WIRE"));
}

}  // namespace two_wire_controller_tests

void runTwoWireControllerTests() {
    using namespace two_wire_controller_tests;
    RUN_TEST(test_config_updates_state_and_configures_service);
    RUN_TEST(test_ensure_configured_prompts_once_then_reapplies_saved_state);
    RUN_TEST(test_sniff_decodes_three_byte_command_frame_and_stops);
    RUN_TEST(test_sniff_reports_start_failure);
    RUN_TEST(test_sniff_formats_non_three_byte_frame_as_response_data);
    RUN_TEST(test_sniff_formats_unknown_three_byte_command);
    RUN_TEST(test_smartcard_command_delegates_to_shell);
    RUN_TEST(test_instruction_reports_not_implemented);
    RUN_TEST(test_release_frees_sniffer_and_resets_configuration_state);
    RUN_TEST(test_unknown_command_displays_help);
}
