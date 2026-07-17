#include <unity.h>

#include "Controllers/ThreeWireController.h"
#include "../Inputs/FakeInput.h"
#include "../Services/FakeThreeWireService.h"
#include "../Shells/FakeShell.h"
#include "../Views/FakeTerminalView.h"

namespace three_wire_controller_tests {

struct ThreeWireControllerFixture {
    FakeTerminalView view;
    FakeInput input;
    ArgTransformer transformer;
    UserInputManager userInput{view, input, transformer};
    FakeThreeWireService service;
    FakeShell eepromShell;
    HelpShell helpShell{view, input, userInput};
    ThreeWireController controller{
        view,
        input,
        userInput,
        service,
        transformer,
        eepromShell,
        helpShell
    };

    ThreeWireControllerFixture() {
        auto& state = GlobalState::getInstance();
        state.setCurrentMode(ModeEnum::ThreeWire);
        state.setThreeWireCsPin(5);
        state.setThreeWireSkPin(18);
        state.setThreeWireDiPin(23);
        state.setThreeWireDoPin(19);
        state.setThreeWireOrg8(false);
        state.setThreeWireEepromModelIndex(0);
    }

    void queueConfig(const std::string& cs = "", const std::string& sk = "",
                     const std::string& di = "", const std::string& doPin = "") {
        input.queueLine(cs);
        input.queueLine(sk);
        input.queueLine(di);
        input.queueLine(doPin);
    }
};

void test_config_updates_state_and_configures_default_eeprom_model() {
    ThreeWireControllerFixture fixture;
    fixture.queueConfig("10", "11", "12", "13");
    GlobalState::getInstance().setThreeWireOrg8(true);

    fixture.controller.handleCommand(TerminalCommand("config"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.configurations.size());
    const auto& config = fixture.service.configurations[0];
    TEST_ASSERT_EQUAL_UINT8(10, config.cs);
    TEST_ASSERT_EQUAL_UINT8(11, config.sk);
    TEST_ASSERT_EQUAL_UINT8(12, config.di);
    TEST_ASSERT_EQUAL_UINT8(13, config.doPin);
    TEST_ASSERT_EQUAL_INT16(46, config.model);
    TEST_ASSERT_TRUE(config.org8);
    TEST_ASSERT_EQUAL_UINT8(10, GlobalState::getInstance().getThreeWireCsPin());
    TEST_ASSERT_TRUE(fixture.view.contains("3WIRE configured"));
}

void test_ensure_configured_prompts_once_then_reapplies_saved_model_and_org() {
    ThreeWireControllerFixture fixture;
    fixture.queueConfig();

    fixture.controller.ensureConfigured();
    GlobalState::getInstance().setThreeWireCsPin(21);
    GlobalState::getInstance().setThreeWireSkPin(22);
    GlobalState::getInstance().setThreeWireDiPin(23);
    GlobalState::getInstance().setThreeWireDoPin(24);
    GlobalState::getInstance().setThreeWireEepromModelIndex(2);
    GlobalState::getInstance().setThreeWireOrg8(true);
    fixture.controller.ensureConfigured();

    TEST_ASSERT_EQUAL_UINT32(2, fixture.service.configurations.size());
    TEST_ASSERT_EQUAL_UINT8(5, fixture.service.configurations[0].cs);
    TEST_ASSERT_EQUAL_INT16(46, fixture.service.configurations[0].model);
    const auto& reapplied = fixture.service.configurations[1];
    TEST_ASSERT_EQUAL_UINT8(21, reapplied.cs);
    TEST_ASSERT_EQUAL_UINT8(22, reapplied.sk);
    TEST_ASSERT_EQUAL_UINT8(23, reapplied.di);
    TEST_ASSERT_EQUAL_UINT8(24, reapplied.doPin);
    TEST_ASSERT_EQUAL_INT16(2, reapplied.model);
    TEST_ASSERT_TRUE(reapplied.org8);
}

void test_eeprom_command_delegates_to_shell() {
    ThreeWireControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("eeprom"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.eepromShell.runCalls);
}

void test_instruction_reports_not_implemented() {
    ThreeWireControllerFixture fixture;

    fixture.controller.handleInstruction({ByteCode(ByteCodeEnum::Start)});

    TEST_ASSERT_TRUE(fixture.view.contains("Instruction handling not yet implemented"));
}

void test_unknown_command_displays_help() {
    ThreeWireControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("wat"));

    TEST_ASSERT_TRUE(fixture.view.contains("Unknown command. Available 3WIRE commands"));
}

}  // namespace three_wire_controller_tests

void runThreeWireControllerTests() {
    using namespace three_wire_controller_tests;
    RUN_TEST(test_config_updates_state_and_configures_default_eeprom_model);
    RUN_TEST(test_ensure_configured_prompts_once_then_reapplies_saved_model_and_org);
    RUN_TEST(test_eeprom_command_delegates_to_shell);
    RUN_TEST(test_instruction_reports_not_implemented);
    RUN_TEST(test_unknown_command_displays_help);
}
