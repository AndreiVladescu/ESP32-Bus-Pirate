#include <unity.h>

#include "Controllers/UsbS3Controller.h"
#include "../Inputs/FakeInput.h"
#include "../Services/FakeUsbS3Service.h"
#include "../Services/FakeUtilityService.h"
#include "../Shells/FakeUsbAdapterShell.h"
#include "../Views/FakeTerminalView.h"

namespace usb_s3_controller_tests {

struct UsbS3ControllerFixture {
    FakeTerminalView view;
    FakeInput terminalInput;
    FakeInput deviceInput;
    FakeUtilityService utility;
    FakeUsbS3Service usb;
    ArgTransformer argTransformer;
    UserInputManager userInput{view, terminalInput, argTransformer};
    HelpShell helpShell{view, terminalInput, userInput};
    FakeUsbAdapterShell adapterShell;
    MouseShell mouseShell{view, terminalInput, userInput, utility};
    UsbS3Controller controller{
        view,
        terminalInput,
        deviceInput,
        utility,
        usb,
        argTransformer,
        userInput,
        helpShell,
        adapterShell,
        mouseShell
    };

    UsbS3ControllerFixture() {
        auto& state = GlobalState::getInstance();
        state.setCurrentMode(ModeEnum::USB);
        state.setTerminalMode(TerminalTypeEnum::Standalone);
        state.setSdCardCsPin(10);
        state.setSdCardClkPin(11);
        state.setSdCardMisoPin(12);
        state.setSdCardMosiPin(13);
    }
};

void test_storage_uses_sdcard_pins_and_reports_success_or_failure() {
    UsbS3ControllerFixture success;
    success.usb.storageActive = true;

    success.controller.handleCommand(TerminalCommand("stick"));

    TEST_ASSERT_EQUAL_UINT32(1, success.usb.storageBeginCalls);
    TEST_ASSERT_EQUAL_UINT8(10, success.usb.storageCalls[0].cs);
    TEST_ASSERT_EQUAL_UINT8(11, success.usb.storageCalls[0].clk);
    TEST_ASSERT_EQUAL_UINT8(12, success.usb.storageCalls[0].miso);
    TEST_ASSERT_EQUAL_UINT8(13, success.usb.storageCalls[0].mosi);
    TEST_ASSERT_TRUE(success.view.contains("USB Stick configured"));

    UsbS3ControllerFixture failure;
    failure.controller.handleCommand(TerminalCommand("storage"));

    TEST_ASSERT_TRUE(failure.view.contains("USB Stick configuration failed"));
}

void test_keyboard_send_configures_hid_and_sends_decoded_text() {
    UsbS3ControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("keyboard", "hello\\nworld"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.usb.configureCalls.size());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.usb.keyboardBeginCalls);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.usb.keyboardStrings.size());
    TEST_ASSERT_EQUAL_STRING("hello\nworld", fixture.usb.keyboardStrings[0].c_str());
    TEST_ASSERT_TRUE(fixture.view.contains("USB Keyboard: String sent"));
}

void test_mouse_move_and_click_delegate_to_usb_service() {
    UsbS3ControllerFixture move;

    move.controller.handleCommand(TerminalCommand("mouse", "move", "12 -5"));

    TEST_ASSERT_EQUAL_UINT32(1, move.usb.configureCalls.size());
    TEST_ASSERT_EQUAL_UINT32(1, move.usb.mouseBeginCalls);
    TEST_ASSERT_EQUAL_UINT32(1, move.usb.mouseMoves.size());
    TEST_ASSERT_EQUAL_INT(12, move.usb.mouseMoves[0].x);
    TEST_ASSERT_EQUAL_INT(-5, move.usb.mouseMoves[0].y);
    TEST_ASSERT_TRUE(move.view.contains("USB Mouse: Moved by (12, -5)"));

    UsbS3ControllerFixture click;
    click.controller.handleCommand(TerminalCommand("mouse", "click"));

    TEST_ASSERT_EQUAL_UINT32(1, click.usb.mouseClicks.size());
    TEST_ASSERT_EQUAL_INT(1, click.usb.mouseClicks[0]);
    TEST_ASSERT_EQUAL_UINT32(1, click.usb.mouseReleases.size());
    TEST_ASSERT_EQUAL_INT(1, click.usb.mouseReleases[0]);
    TEST_ASSERT_EQUAL_UINT32(1, click.utility.sleepMsCalls);
    TEST_ASSERT_EQUAL_UINT32(100, click.utility.lastSleepMs);
    TEST_ASSERT_TRUE(click.view.contains("USB Mouse: Click sent"));
}

void test_mouse_rejects_invalid_move_arguments() {
    UsbS3ControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("mouse", "move", "x 2"));

    TEST_ASSERT_TRUE(fixture.usb.mouseMoves.empty());
    TEST_ASSERT_TRUE(fixture.view.contains("Usage: mouse move <x> <y>"));
}

void test_gamepad_sends_known_button_and_rejects_unknown_button() {
    UsbS3ControllerFixture sent;

    sent.controller.handleCommand(TerminalCommand("gamepad", "A"));

    TEST_ASSERT_EQUAL_UINT32(1, sent.usb.configureCalls.size());
    TEST_ASSERT_EQUAL_UINT32(1, sent.usb.gamepadBeginCalls);
    TEST_ASSERT_EQUAL_UINT32(1, sent.usb.gamepadPresses.size());
    TEST_ASSERT_EQUAL_STRING("a", sent.usb.gamepadPresses[0].c_str());
    TEST_ASSERT_TRUE(sent.view.contains("USB Gamepad: Key sent"));

    UsbS3ControllerFixture rejected;
    rejected.controller.handleCommand(TerminalCommand("gamepad", "turbo"));

    TEST_ASSERT_TRUE(rejected.usb.gamepadPresses.empty());
    TEST_ASSERT_TRUE(rejected.view.contains("Usage: gamepad"));
}

void test_system_control_sleep_wake_and_poweroff() {
    UsbS3ControllerFixture sleep;
    sleep.controller.handleCommand(TerminalCommand("sysctrl", "sleep"));

    TEST_ASSERT_EQUAL_UINT32(1, sleep.usb.configureCalls.size());
    TEST_ASSERT_EQUAL_UINT32(1, sleep.usb.systemControlBeginCalls);
    TEST_ASSERT_EQUAL_UINT32(1, sleep.usb.systemSleepCalls);

    UsbS3ControllerFixture wake;
    wake.controller.handleCommand(TerminalCommand("sysctrl", "wake"));

    TEST_ASSERT_EQUAL_UINT32(1, wake.usb.systemWakeCalls);

    UsbS3ControllerFixture off;
    off.controller.handleCommand(TerminalCommand("sysctrl", "off"));

    TEST_ASSERT_EQUAL_UINT32(1, off.usb.systemPowerOffCalls);
    TEST_ASSERT_EQUAL_UINT32(10, off.usb.lastPowerOffHoldMs);
}

void test_host_can_be_cancelled_or_started_and_stopped_immediately() {
    UsbS3ControllerFixture cancelled;
    cancelled.terminalInput.queueLine("n");

    cancelled.controller.handleCommand(TerminalCommand("host"));

    TEST_ASSERT_EQUAL_UINT32(0, cancelled.usb.usbHostBeginCalls);
    TEST_ASSERT_TRUE(cancelled.view.contains("Action cancelled"));

    UsbS3ControllerFixture started;
    started.terminalInput.queueLine("y");

    started.controller.handleCommand(TerminalCommand("host"));

    TEST_ASSERT_EQUAL_UINT32(1, started.usb.usbHostBeginCalls);
    TEST_ASSERT_TRUE(started.view.contains("USB Host: Stopped by user"));
}

void test_adapter_reset_config_and_help_paths() {
    UsbS3ControllerFixture adapter;
    adapter.controller.handleCommand(TerminalCommand("adapter"));

    TEST_ASSERT_EQUAL_UINT32(1, adapter.adapterShell.runCalls);

    UsbS3ControllerFixture reset;
    reset.controller.handleCommand(TerminalCommand("reset"));

    TEST_ASSERT_EQUAL_UINT32(1, reset.usb.resetCalls);
    TEST_ASSERT_TRUE(reset.view.contains("USB Reset"));

    UsbS3ControllerFixture config;
    config.terminalInput.queueLine("y");
    config.terminalInput.queueLine("20");
    config.terminalInput.queueLine("21");
    config.terminalInput.queueLine("22");
    config.terminalInput.queueLine("23");
    config.controller.handleCommand(TerminalCommand("config"));

    TEST_ASSERT_EQUAL_UINT8(20, GlobalState::getInstance().getSdCardCsPin());
    TEST_ASSERT_EQUAL_UINT8(21, GlobalState::getInstance().getSdCardClkPin());
    TEST_ASSERT_EQUAL_UINT8(22, GlobalState::getInstance().getSdCardMisoPin());
    TEST_ASSERT_EQUAL_UINT8(23, GlobalState::getInstance().getSdCardMosiPin());
    TEST_ASSERT_TRUE(config.view.contains("USB Configured"));

    UsbS3ControllerFixture help;
    help.controller.handleCommand(TerminalCommand("wat"));

    TEST_ASSERT_TRUE(help.view.contains("Unknown command. Available USB commands"));
    TEST_ASSERT_TRUE(help.view.contains("USB"));
}

}  // namespace usb_s3_controller_tests

void runUsbS3ControllerTests() {
    using namespace usb_s3_controller_tests;
    RUN_TEST(test_storage_uses_sdcard_pins_and_reports_success_or_failure);
    RUN_TEST(test_keyboard_send_configures_hid_and_sends_decoded_text);
    RUN_TEST(test_mouse_move_and_click_delegate_to_usb_service);
    RUN_TEST(test_mouse_rejects_invalid_move_arguments);
    RUN_TEST(test_gamepad_sends_known_button_and_rejects_unknown_button);
    RUN_TEST(test_system_control_sleep_wake_and_poweroff);
    RUN_TEST(test_host_can_be_cancelled_or_started_and_stopped_immediately);
    RUN_TEST(test_adapter_reset_config_and_help_paths);
}
