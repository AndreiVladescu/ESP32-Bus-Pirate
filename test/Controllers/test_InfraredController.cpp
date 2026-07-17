#include <unity.h>

#include "Controllers/InfraredController.h"
#include "../Inputs/FakeInput.h"
#include "../Services/FakeI2cService.h"
#include "../Services/FakeInfraredService.h"
#include "../Services/FakeLittleFsService.h"
#include "../Services/FakeUtilityService.h"
#include "../Shells/FakeShell.h"
#include "../Views/FakeDeviceView.h"
#include "../Views/FakeTerminalView.h"

namespace infrared_controller_tests {

struct InfraredControllerFixture {
    FakeTerminalView view;
    FakeInput input;
    FakeDeviceView device;
    FakeUtilityService utility;
    FakeInfraredService service;
    FakeLittleFsService littleFs;
    FakeI2cService i2c;
    ArgTransformer argTransformer;
    InfraredRemoteTransformer remoteTransformer;
    UserInputManager userInput{view, input, argTransformer};
    FakeShell remoteShell;
    HelpShell helpShell{view, input, userInput};
    InfraredController controller{
        view,
        input,
        device,
        utility,
        service,
        littleFs,
        i2c,
        argTransformer,
        remoteTransformer,
        userInput,
        remoteShell,
        helpShell
    };

    InfraredControllerFixture() {
        auto& state = GlobalState::getInstance();
        state.setCurrentMode(ModeEnum::Infrared);
        state.setTerminalMode(TerminalTypeEnum::Standalone);
        state.setInfraredTxPin(44);
        state.setInfraredRxPin(1);
        state.setInfraredProtocol(InfraredProtocolEnum::_NEC);
    }
};

void test_config_updates_selected_pins_and_configures_service() {
    InfraredControllerFixture fixture;
    fixture.input.queueLine("12");
    fixture.input.queueLine("13");

    fixture.controller.handleCommand(TerminalCommand("config"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.configurations.size());
    TEST_ASSERT_EQUAL_UINT8(12, fixture.service.configurations[0].tx);
    TEST_ASSERT_EQUAL_UINT8(13, fixture.service.configurations[0].rx);
    TEST_ASSERT_EQUAL_UINT8(12, GlobalState::getInstance().getInfraredTxPin());
    TEST_ASSERT_EQUAL_UINT8(13, GlobalState::getInstance().getInfraredRxPin());
    TEST_ASSERT_TRUE(fixture.view.contains("Infrared configured"));
}

void test_ensure_configured_prompts_once_then_reapplies_saved_pins() {
    InfraredControllerFixture fixture;
    fixture.input.queueLine("14");
    fixture.input.queueLine("15");

    fixture.controller.ensureConfigured();
    GlobalState::getInstance().setInfraredTxPin(16);
    GlobalState::getInstance().setInfraredRxPin(17);
    fixture.controller.ensureConfigured();

    TEST_ASSERT_EQUAL_UINT32(2, fixture.service.configurations.size());
    TEST_ASSERT_EQUAL_UINT8(14, fixture.service.configurations[0].tx);
    TEST_ASSERT_EQUAL_UINT8(15, fixture.service.configurations[0].rx);
    TEST_ASSERT_EQUAL_UINT8(16, fixture.service.configurations[1].tx);
    TEST_ASSERT_EQUAL_UINT8(17, fixture.service.configurations[1].rx);
}

void test_send_parses_device_subdevice_command_and_repeats_three_times() {
    InfraredControllerFixture fixture;
    GlobalState::getInstance().setInfraredProtocol(InfraredProtocolEnum::_RC5);

    fixture.controller.handleCommand(TerminalCommand("send", "1", "2 3"));

    TEST_ASSERT_EQUAL_UINT32(3, fixture.service.sentCommands.size());
    TEST_ASSERT_EQUAL(InfraredProtocolEnum::_RC5, fixture.service.sentCommands[0].getProtocol());
    TEST_ASSERT_EQUAL_INT(1, fixture.service.sentCommands[0].getDevice());
    TEST_ASSERT_EQUAL_INT(2, fixture.service.sentCommands[0].getSubdevice());
    TEST_ASSERT_EQUAL_INT(3, fixture.service.sentCommands[0].getFunction());
    TEST_ASSERT_EQUAL_UINT32(3, fixture.utility.sleepMsCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("IR command sent with protocol"));
}

void test_send_rejects_missing_or_invalid_numbers_without_transmitting() {
    InfraredControllerFixture missing;
    missing.controller.handleCommand(TerminalCommand("send", "1", "2"));

    InfraredControllerFixture invalid;
    invalid.controller.handleCommand(TerminalCommand("send", "AA", "2 3"));

    TEST_ASSERT_TRUE(missing.service.sentCommands.empty());
    TEST_ASSERT_TRUE(invalid.service.sentCommands.empty());
    TEST_ASSERT_TRUE(missing.view.contains("Missing argument"));
    TEST_ASSERT_TRUE(invalid.view.contains("Invalid number format"));
}

void test_receive_decoded_command_displays_fields_and_updates_device_view() {
    InfraredControllerFixture fixture;
    fixture.service.queueDecoded(InfraredProtocolEnum::_NEC, 12, 34, 56);
    fixture.input.queueReadChar(KEY_NONE);
    fixture.input.queueReadChar('\n');

    fixture.controller.handleCommand(TerminalCommand("receive"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.startReceiverCalls);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.stopReceiverCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("Infrared signal received"));
    TEST_ASSERT_TRUE(fixture.view.contains("Device   : 12"));
    TEST_ASSERT_TRUE(fixture.view.contains("Command  : 56"));
    TEST_ASSERT_EQUAL_UINT32(1, fixture.device.shownConfigs.size());
    TEST_ASSERT_EQUAL_STRING("RECV", fixture.device.shownConfigs[0].getMode().c_str());
}

void test_receive_raw_displays_timings_without_decoding() {
    InfraredControllerFixture fixture;
    fixture.input.queueLine("n");
    fixture.input.queueReadChar(KEY_NONE);
    fixture.input.queueReadChar('\n');
    fixture.service.queueRaw({9000, 4500, 560}, 38);

    fixture.controller.handleCommand(TerminalCommand("receive"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.startReceiverCalls);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.stopReceiverCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("RAW Timings"));
    TEST_ASSERT_TRUE(fixture.view.contains("+9000 -4500 +560"));
}

void test_remote_command_delegates_to_shell() {
    InfraredControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("remote"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.remoteShell.runCalls);
}

void test_record_saves_selected_decoded_command_to_littlefs() {
    InfraredControllerFixture fixture;
    fixture.service.queueDecoded(InfraredProtocolEnum::_NEC, 4, 1, 8);
    fixture.input.queueReadChar(KEY_NONE);
    fixture.input.queueReadChar('\n');
    fixture.input.queueLine("y");
    fixture.input.queueLine("power");
    fixture.input.queueLine("tv_remote");

    fixture.controller.handleCommand(TerminalCommand("record"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.startReceiverCalls);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.stopReceiverCalls);
    TEST_ASSERT_EQUAL_STRING("/tv_remote.ir", fixture.littleFs.lastWritePath.c_str());
    TEST_ASSERT_TRUE(fixture.littleFs.lastWriteData.find("Filetype: IR") != std::string::npos);
    TEST_ASSERT_TRUE(fixture.littleFs.lastWriteData.find("name: power") != std::string::npos);
    TEST_ASSERT_TRUE(fixture.view.contains("INFRARED Record: Saved file"));
}

void test_record_refuses_when_littlefs_has_not_enough_space() {
    InfraredControllerFixture fixture;
    fixture.littleFs.freeBytesValue = 1024;

    fixture.controller.handleCommand(TerminalCommand("record"));

    TEST_ASSERT_EQUAL_UINT32(0, fixture.service.startReceiverCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("Not enough LittleFS space"));
}

void test_load_sends_selected_command_from_littlefs_file() {
    InfraredControllerFixture fixture;
    InfraredFileRemoteCommand command{};
    command.functionName = "power";
    command.protocol = InfraredProtocolEnum::_RC5;
    command.address = 0x0104;
    command.function = 0x08;
    fixture.littleFs.files["/tv.ir"] = fixture.remoteTransformer.transformToFileFormat("tv", {command});
    fixture.input.queueLine("1");
    fixture.input.queueLine("1");
    fixture.input.queueLine("2");

    fixture.controller.handleCommand(TerminalCommand("load"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.sentFileCommands.size());
    TEST_ASSERT_EQUAL_STRING("power", fixture.service.sentFileCommands[0].functionName.c_str());
    TEST_ASSERT_EQUAL(InfraredProtocolEnum::_RC5, fixture.service.sentFileCommands[0].protocol);
    TEST_ASSERT_EQUAL_HEX16(0x0104, fixture.service.sentFileCommands[0].address);
    TEST_ASSERT_EQUAL_HEX8(0x08, fixture.service.sentFileCommands[0].function);
    TEST_ASSERT_TRUE(fixture.view.contains("Sent command 'power'"));
}

void test_load_reports_missing_ir_files() {
    InfraredControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("load"));

    TEST_ASSERT_TRUE(fixture.view.contains("No .ir files found"));
}

void test_jam_selects_mode_carrier_and_density_then_stops() {
    InfraredControllerFixture fixture;
    fixture.input.queueLine("2");
    fixture.input.queueLine("");
    fixture.input.queueLine("7");
    fixture.input.queueReadChar(KEY_NONE);
    fixture.input.queueReadChar('\n');

    fixture.controller.handleCommand(TerminalCommand("jam"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.sendJamCalls);
    TEST_ASSERT_EQUAL_UINT8(1, fixture.service.lastJamMode);
    TEST_ASSERT_EQUAL_UINT16(38, fixture.service.lastJamKhz);
    TEST_ASSERT_EQUAL_UINT8(7, fixture.service.lastJamDensity);
    TEST_ASSERT_TRUE(fixture.view.contains("INFRARED Jam: Stopped by user"));
}

void test_unknown_command_displays_help() {
    InfraredControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("wat"));

    TEST_ASSERT_TRUE(fixture.view.contains("Unknown command. Available INFRARED commands"));
}

}  // namespace infrared_controller_tests

void runInfraredControllerTests() {
    using namespace infrared_controller_tests;
    RUN_TEST(test_config_updates_selected_pins_and_configures_service);
    RUN_TEST(test_ensure_configured_prompts_once_then_reapplies_saved_pins);
    RUN_TEST(test_send_parses_device_subdevice_command_and_repeats_three_times);
    RUN_TEST(test_send_rejects_missing_or_invalid_numbers_without_transmitting);
    RUN_TEST(test_receive_decoded_command_displays_fields_and_updates_device_view);
    RUN_TEST(test_receive_raw_displays_timings_without_decoding);
    RUN_TEST(test_remote_command_delegates_to_shell);
    RUN_TEST(test_record_saves_selected_decoded_command_to_littlefs);
    RUN_TEST(test_record_refuses_when_littlefs_has_not_enough_space);
    RUN_TEST(test_load_sends_selected_command_from_littlefs_file);
    RUN_TEST(test_load_reports_missing_ir_files);
    RUN_TEST(test_jam_selects_mode_carrier_and_density_then_stops);
    RUN_TEST(test_unknown_command_displays_help);
}
