#include <unity.h>

#include "Controllers/CellController.h"
#include "../Inputs/FakeInput.h"
#include "../Services/FakeCellService.h"
#include "../Services/FakeUtilityService.h"
#include "../Shells/FakeShell.h"
#include "../Views/FakeTerminalView.h"

namespace cell_controller_tests {

struct CellControllerFixture {
    FakeTerminalView view;
    FakeInput input;
    FakeUtilityService utility;
    FakeCellService service;
    ArgTransformer argTransformer;
    AtTransformer atTransformer;
    UserInputManager userInput{view, input, argTransformer};
    HelpShell helpShell{view, input, userInput};
    FakeShell callShell;
    FakeShell smsShell;
    CellController controller{
        view,
        input,
        utility,
        service,
        argTransformer,
        atTransformer,
        userInput,
        helpShell,
        callShell,
        smsShell
    };

    CellControllerFixture() {
        auto& state = GlobalState::getInstance();
        state.setCurrentMode(ModeEnum::CELL);
        state.setTerminalMode(TerminalTypeEnum::Standalone);
        state.setUartRxPin(1);
        state.setUartTxPin(2);
        state.setUartBaudRate(9600);
    }

    void configureSuccessfully(uint8_t rx = 7, uint8_t tx = 8, uint32_t baud = 115200) {
        input.queueLine(std::to_string(rx));
        input.queueLine(std::to_string(tx));
        input.queueLine(std::to_string(baud));
        controller.handleCommand(TerminalCommand("config"));
        view.output.clear();
        view.printCalls.clear();
        view.printlnCalls.clear();
    }
};

void test_config_initializes_modem_and_displays_module_info() {
    CellControllerFixture fixture;
    fixture.input.queueLine("7");
    fixture.input.queueLine("8");
    fixture.input.queueLine("115200");

    fixture.controller.handleCommand(TerminalCommand("config"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.initCalls.size());
    TEST_ASSERT_EQUAL_UINT8(7, fixture.service.initCalls[0].rxPin);
    TEST_ASSERT_EQUAL_UINT8(8, fixture.service.initCalls[0].txPin);
    TEST_ASSERT_EQUAL_UINT32(115200, fixture.service.initCalls[0].baudrate);
    TEST_ASSERT_EQUAL_UINT8(7, GlobalState::getInstance().getUartRxPin());
    TEST_ASSERT_EQUAL_UINT8(8, GlobalState::getInstance().getUartTxPin());
    TEST_ASSERT_EQUAL_UINT32(115200, GlobalState::getInstance().getUartBaudRate());
    TEST_ASSERT_TRUE(fixture.view.contains("Modem detected"));
    TEST_ASSERT_TRUE(fixture.view.contains("Quectel EC25"));
}

void test_config_reports_missing_modem_after_init() {
    CellControllerFixture fixture;
    fixture.service.detectResult = false;
    fixture.input.queueLine("7");
    fixture.input.queueLine("8");
    fixture.input.queueLine("115200");

    fixture.controller.handleCommand(TerminalCommand("config"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.initCalls.size());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.detectCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("No modem detected"));
}

void test_ensure_configured_reapplies_saved_uart_state() {
    CellControllerFixture fixture;
    fixture.input.queueLine("7");
    fixture.input.queueLine("8");
    fixture.input.queueLine("115200");

    fixture.controller.ensureConfigured();
    GlobalState::getInstance().setUartRxPin(10);
    GlobalState::getInstance().setUartTxPin(11);
    GlobalState::getInstance().setUartBaudRate(57600);
    fixture.controller.ensureConfigured();

    TEST_ASSERT_EQUAL_UINT32(2, fixture.service.initCalls.size());
    TEST_ASSERT_EQUAL_UINT8(7, fixture.service.initCalls[0].rxPin);
    TEST_ASSERT_EQUAL_UINT8(8, fixture.service.initCalls[0].txPin);
    TEST_ASSERT_EQUAL_UINT32(115200, fixture.service.initCalls[0].baudrate);
    TEST_ASSERT_EQUAL_UINT8(10, fixture.service.initCalls[1].rxPin);
    TEST_ASSERT_EQUAL_UINT8(11, fixture.service.initCalls[1].txPin);
    TEST_ASSERT_EQUAL_UINT32(57600, fixture.service.initCalls[1].baudrate);
}

void test_modem_displays_identity_status_and_clock() {
    CellControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("modem"));

    TEST_ASSERT_TRUE(fixture.view.contains("MODEM INFO"));
    TEST_ASSERT_TRUE(fixture.view.contains("Manufacturer"));
    TEST_ASSERT_TRUE(fixture.view.contains("Model: EC25"));
    TEST_ASSERT_TRUE(fixture.view.contains("IMEI"));
}

void test_sim_displays_sim_details_and_unlock_hint_when_not_ready() {
    CellControllerFixture fixture;
    fixture.service.simReady = false;
    fixture.service.simState = "+CPIN: SIM PIN\r\nOK\r\n";

    fixture.controller.handleCommand(TerminalCommand("sim"));

    TEST_ASSERT_TRUE(fixture.view.contains("SIM CARD"));
    TEST_ASSERT_TRUE(fixture.view.contains("SIM state"));
    TEST_ASSERT_TRUE(fixture.view.contains("Use 'unlock' command"));
}

void test_network_displays_signal_registration_and_pdp_status() {
    CellControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("network"));

    TEST_ASSERT_TRUE(fixture.view.contains("NETWORK STATUS"));
    TEST_ASSERT_TRUE(fixture.view.contains("Signal"));
    TEST_ASSERT_TRUE(fixture.view.contains("CS reg"));
    TEST_ASSERT_TRUE(fixture.view.contains("PDP"));
}

void test_unlock_pin_validates_confirms_and_reports_new_sim_state() {
    CellControllerFixture fixture;
    fixture.service.simReady = false;
    fixture.input.queueLine("y");

    fixture.controller.handleCommand(TerminalCommand("unlock", "1234"));

    TEST_ASSERT_EQUAL_STRING("1234", fixture.service.lastPin.c_str());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.utility.sleepMsCalls);
    TEST_ASSERT_EQUAL_UINT32(200, fixture.utility.lastSleepMs);
    TEST_ASSERT_TRUE(fixture.view.contains("PIN accepted"));
    TEST_ASSERT_TRUE(fixture.view.contains("SIM state"));
}

void test_unlock_rejects_invalid_pin_without_service_call() {
    CellControllerFixture fixture;
    fixture.service.simReady = false;

    fixture.controller.handleCommand(TerminalCommand("unlock", "12"));

    TEST_ASSERT_TRUE(fixture.service.lastPin.empty());
    TEST_ASSERT_TRUE(fixture.view.contains("Invalid PIN format"));
}

void test_unlock_puk_flow_requires_confirmation_and_new_pin() {
    CellControllerFixture fixture;
    fixture.service.pukRequired = true;
    fixture.input.queueLine("y");
    fixture.input.queueLine("12345678");
    fixture.input.queueLine("4321");
    fixture.input.queueLine("y");

    fixture.controller.handleCommand(TerminalCommand("unlock"));

    TEST_ASSERT_EQUAL_STRING("12345678", fixture.service.lastPuk.c_str());
    TEST_ASSERT_EQUAL_STRING("4321", fixture.service.lastNewPin.c_str());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.utility.sleepMsCalls);
    TEST_ASSERT_EQUAL_UINT32(100, fixture.utility.lastSleepMs);
    TEST_ASSERT_TRUE(fixture.view.contains("PUK accepted"));
}

void test_sms_and_call_require_ready_sim_then_delegate_to_shells() {
    CellControllerFixture smsBlocked;
    smsBlocked.service.simReady = false;
    smsBlocked.controller.handleCommand(TerminalCommand("sms"));

    CellControllerFixture callBlocked;
    callBlocked.service.simReady = false;
    callBlocked.controller.handleCommand(TerminalCommand("call"));

    CellControllerFixture allowed;
    allowed.controller.handleCommand(TerminalCommand("sms"));
    allowed.controller.handleCommand(TerminalCommand("call"));

    TEST_ASSERT_EQUAL_UINT32(0, smsBlocked.smsShell.runCalls);
    TEST_ASSERT_EQUAL_UINT32(0, callBlocked.callShell.runCalls);
    TEST_ASSERT_TRUE(smsBlocked.view.contains("SIM not ready"));
    TEST_ASSERT_TRUE(callBlocked.view.contains("SIM not ready"));
    TEST_ASSERT_EQUAL_UINT32(1, allowed.smsShell.runCalls);
    TEST_ASSERT_EQUAL_UINT32(1, allowed.callShell.runCalls);
}

void test_ussd_sends_valid_code_with_default_dcs_and_rejects_invalid_code() {
    CellControllerFixture fixture;
    fixture.controller.handleCommand(TerminalCommand("ussd", "*123#"));

    CellControllerFixture invalid;
    invalid.controller.handleCommand(TerminalCommand("ussd", "abc!"));

    TEST_ASSERT_EQUAL_STRING("*123#", fixture.service.lastUssdCode.c_str());
    TEST_ASSERT_EQUAL_UINT8(15, fixture.service.lastUssdDcs);
    TEST_ASSERT_TRUE(fixture.view.contains("USSD request: OK"));
    TEST_ASSERT_TRUE(invalid.service.lastUssdCode.empty());
    TEST_ASSERT_TRUE(invalid.view.contains("Invalid USSD code"));
}

void test_operator_scan_can_reset_to_auto_after_configuration() {
    CellControllerFixture fixture;
    fixture.configureSuccessfully();
    fixture.input.queueLine("y");

    fixture.controller.handleCommand(TerminalCommand("operator"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.setOperatorAutoCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("Orange F"));
    TEST_ASSERT_TRUE(fixture.view.contains("Set operator auto: OK"));
}

void test_phonebook_dumps_storage_caps_and_entries_after_configuration() {
    CellControllerFixture fixture;
    fixture.configureSuccessfully();

    fixture.controller.handleCommand(TerminalCommand("phonebook"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.phonebookReadRangeCalls);
    TEST_ASSERT_EQUAL_UINT16(1, fixture.service.lastPhonebookStart);
    TEST_ASSERT_EQUAL_UINT16(250, fixture.service.lastPhonebookEnd);
    TEST_ASSERT_TRUE(fixture.view.contains("PHONEBOOK"));
    TEST_ASSERT_TRUE(fixture.view.contains("Alice"));
}

void test_setmode_applies_airplane_mode_and_displays_registration() {
    CellControllerFixture fixture;
    fixture.input.queueLine("2");

    fixture.controller.handleCommand(TerminalCommand("setmode"));

    TEST_ASSERT_EQUAL_UINT8(4, fixture.service.lastFunctionality);
    TEST_ASSERT_TRUE(fixture.view.contains("CFUN=4: OK"));
    TEST_ASSERT_TRUE(fixture.view.contains("CS reg"));
}

void test_setmode_default_choice_exits_without_touching_modem() {
    CellControllerFixture fixture;
    fixture.input.queueLine("");

    fixture.controller.handleCommand(TerminalCommand("setmode"));

    TEST_ASSERT_EQUAL_UINT8(0, fixture.service.lastFunctionality);
    TEST_ASSERT_TRUE(fixture.view.contains("Exit setmode"));
}

}  // namespace cell_controller_tests

void runCellControllerTests() {
    using namespace cell_controller_tests;
    RUN_TEST(test_config_initializes_modem_and_displays_module_info);
    RUN_TEST(test_config_reports_missing_modem_after_init);
    RUN_TEST(test_ensure_configured_reapplies_saved_uart_state);
    RUN_TEST(test_modem_displays_identity_status_and_clock);
    RUN_TEST(test_sim_displays_sim_details_and_unlock_hint_when_not_ready);
    RUN_TEST(test_network_displays_signal_registration_and_pdp_status);
    RUN_TEST(test_unlock_pin_validates_confirms_and_reports_new_sim_state);
    RUN_TEST(test_unlock_rejects_invalid_pin_without_service_call);
    RUN_TEST(test_unlock_puk_flow_requires_confirmation_and_new_pin);
    RUN_TEST(test_sms_and_call_require_ready_sim_then_delegate_to_shells);
    RUN_TEST(test_ussd_sends_valid_code_with_default_dcs_and_rejects_invalid_code);
    RUN_TEST(test_operator_scan_can_reset_to_auto_after_configuration);
    RUN_TEST(test_phonebook_dumps_storage_caps_and_entries_after_configuration);
    RUN_TEST(test_setmode_applies_airplane_mode_and_displays_registration);
    RUN_TEST(test_setmode_default_choice_exits_without_touching_modem);
}
