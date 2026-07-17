#include <unity.h>

#include "Controllers/BluetoothController.h"
#include "../Inputs/FakeInput.h"
#include "../Services/FakeBluetoothService.h"
#include "../Services/FakeUtilityService.h"
#include "../Views/FakeTerminalView.h"

namespace bluetooth_controller_tests {

struct BluetoothControllerFixture {
    FakeTerminalView view;
    FakeInput terminalInput;
    FakeInput deviceInput;
    FakeUtilityService utility;
    FakeBluetoothService service;
    ArgTransformer argTransformer;
    UserInputManager userInput{view, terminalInput, argTransformer};
    HelpShell helpShell{view, terminalInput, userInput};
    MouseShell mouseShell{view, terminalInput, userInput, utility};
    BluetoothController controller{
        view,
        terminalInput,
        deviceInput,
        utility,
        service,
        argTransformer,
        userInput,
        helpShell,
        mouseShell
    };

    BluetoothControllerFixture() {
        auto& state = GlobalState::getInstance();
        state.setCurrentMode(ModeEnum::Bluetooth);
        state.setTerminalMode(TerminalTypeEnum::Standalone);
    }
};

void test_scan_reports_devices_and_empty_results() {
    BluetoothControllerFixture withDevices;
    withDevices.service.scanResults = {"Keyboard AA:BB", "Mouse 11:22"};
    withDevices.controller.handleCommand(TerminalCommand("scan"));

    BluetoothControllerFixture empty;
    empty.controller.handleCommand(TerminalCommand("scan"));

    TEST_ASSERT_EQUAL_INT(10, withDevices.service.lastScanSeconds);
    TEST_ASSERT_TRUE(withDevices.view.contains("Keyboard AA:BB"));
    TEST_ASSERT_TRUE(withDevices.view.contains("Mouse 11:22"));
    TEST_ASSERT_TRUE(empty.view.contains("No devices"));
}

void test_pair_requires_address_and_reports_discovered_services() {
    BluetoothControllerFixture missing;
    missing.controller.handleCommand(TerminalCommand("pair"));

    BluetoothControllerFixture success;
    success.service.connectResults = {"180F", "1812"};
    success.controller.handleCommand(TerminalCommand("pair", "AA:BB:CC:DD:EE:FF"));

    TEST_ASSERT_TRUE(missing.view.contains("Usage: pair"));
    TEST_ASSERT_EQUAL(BluetoothMode::CLIENT, success.service.mode);
    TEST_ASSERT_EQUAL_UINT32(1, success.service.connectAddresses.size());
    TEST_ASSERT_EQUAL_STRING("AA:BB:CC:DD:EE:FF", success.service.connectAddresses[0].c_str());
    TEST_ASSERT_TRUE(success.view.contains("Successfully connected"));
    TEST_ASSERT_TRUE(success.view.contains("180F"));
    TEST_ASSERT_TRUE(success.view.contains("1812"));
}

void test_pair_reports_connection_failure() {
    BluetoothControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("pair", "AA:BB"));

    TEST_ASSERT_EQUAL(BluetoothMode::CLIENT, fixture.service.mode);
    TEST_ASSERT_TRUE(fixture.view.contains("Failed to connect"));
}

void test_status_reports_uninitialized_client_and_server_states() {
    BluetoothControllerFixture none;
    none.controller.handleCommand(TerminalCommand("status"));

    BluetoothControllerFixture client;
    client.service.mode = BluetoothMode::CLIENT;
    client.service.connected = true;
    client.controller.handleCommand(TerminalCommand("status"));

    BluetoothControllerFixture server;
    server.service.mode = BluetoothMode::SERVER;
    server.service.connected = false;
    server.service.macAddress = "";
    server.controller.handleCommand(TerminalCommand("status"));

    TEST_ASSERT_TRUE(none.view.contains("Not initialized"));
    TEST_ASSERT_TRUE(client.view.contains("Mode: Client"));
    TEST_ASSERT_TRUE(client.view.contains("Connected: Yes"));
    TEST_ASSERT_TRUE(client.view.contains("AA:BB:CC:DD:EE:FF"));
    TEST_ASSERT_TRUE(server.view.contains("Mode: Server"));
    TEST_ASSERT_TRUE(server.view.contains("MAC Address: Unknown"));
}

void test_server_starts_default_or_custom_name_and_skips_when_already_connected() {
    BluetoothControllerFixture defaultName;
    defaultName.controller.handleCommand(TerminalCommand("server"));

    BluetoothControllerFixture customName;
    customName.controller.handleCommand(TerminalCommand("server", "Pad"));

    BluetoothControllerFixture already;
    already.service.mode = BluetoothMode::SERVER;
    already.service.connected = true;
    already.controller.handleCommand(TerminalCommand("server", "Ignored"));

    TEST_ASSERT_EQUAL_UINT32(1, defaultName.service.startedServers.size());
    TEST_ASSERT_EQUAL_STRING("Bit-Pirate-Bluetooth", defaultName.service.startedServers[0].c_str());
    TEST_ASSERT_EQUAL_UINT32(1, customName.service.startedServers.size());
    TEST_ASSERT_EQUAL_STRING("Pad", customName.service.startedServers[0].c_str());
    TEST_ASSERT_TRUE(already.service.startedServers.empty());
    TEST_ASSERT_TRUE(already.view.contains("Already Started"));
}

void test_keyboard_requires_server_and_sends_decoded_text() {
    BluetoothControllerFixture blocked;
    blocked.controller.handleCommand(TerminalCommand("keyboard", "hello"));

    BluetoothControllerFixture allowed;
    allowed.service.mode = BluetoothMode::SERVER;
    allowed.controller.handleCommand(TerminalCommand("keyboard", "hello\\nworld"));

    TEST_ASSERT_TRUE(blocked.service.keyboardTexts.empty());
    TEST_ASSERT_TRUE(blocked.view.contains("Start the server"));
    TEST_ASSERT_EQUAL_UINT32(1, allowed.service.keyboardTexts.size());
    TEST_ASSERT_EQUAL_STRING("hello\nworld", allowed.service.keyboardTexts[0].c_str());
    TEST_ASSERT_TRUE(allowed.view.contains("String sent"));
}

void test_keyboard_bridge_forwards_terminal_keys_until_device_input_stops() {
    BluetoothControllerFixture fixture;
    fixture.service.mode = BluetoothMode::SERVER;
    fixture.deviceInput.queueReadChar(KEY_NONE);
    fixture.terminalInput.queueReadChar('a');

    fixture.controller.handleCommand(TerminalCommand("keyboard", "bridge"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.keyboardTexts.size());
    TEST_ASSERT_EQUAL_STRING("a", fixture.service.keyboardTexts[0].c_str());
    TEST_ASSERT_TRUE(fixture.view.contains("Keyboard Bridge: Stopped"));
}

void test_mouse_click_and_move_require_server_then_delegate() {
    BluetoothControllerFixture blocked;
    blocked.controller.handleCommand(TerminalCommand("mouse", "click"));

    BluetoothControllerFixture click;
    click.service.mode = BluetoothMode::SERVER;
    click.controller.handleCommand(TerminalCommand("mouse", "click"));

    BluetoothControllerFixture move;
    move.service.mode = BluetoothMode::SERVER;
    move.controller.handleCommand(TerminalCommand("mouse", "move", "10 -20"));

    TEST_ASSERT_EQUAL_UINT32(0, blocked.service.clickCalls);
    TEST_ASSERT_TRUE(blocked.view.contains("Start the server"));
    TEST_ASSERT_EQUAL_UINT32(1, click.service.clickCalls);
    TEST_ASSERT_EQUAL_UINT32(1, move.service.mouseMoves.size());
    TEST_ASSERT_EQUAL_INT16(10, move.service.mouseMoves[0].first);
    TEST_ASSERT_EQUAL_INT16(-20, move.service.mouseMoves[0].second);
}

void test_mouse_jiggle_moves_once_then_stops_on_enter() {
    BluetoothControllerFixture fixture;
    fixture.service.mode = BluetoothMode::SERVER;
    fixture.utility.randomRangeValue = 0;

    fixture.controller.handleCommand(TerminalCommand("mouse", "jiggle", "1"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.mouseMoves.size());
    TEST_ASSERT_EQUAL_INT16(1, fixture.service.mouseMoves[0].first);
    TEST_ASSERT_EQUAL_INT16(0, fixture.service.mouseMoves[0].second);
    TEST_ASSERT_TRUE(fixture.view.contains("Jiggle stopped"));
}

void test_spoof_requires_uninitialized_adapter_and_reports_result() {
    BluetoothControllerFixture missing;
    missing.controller.handleCommand(TerminalCommand("spoof"));

    BluetoothControllerFixture busy;
    busy.service.mode = BluetoothMode::SERVER;
    busy.controller.handleCommand(TerminalCommand("spoof", "11:22:33:44:55:66"));

    BluetoothControllerFixture success;
    success.controller.handleCommand(TerminalCommand("spoof", "11:22:33:44:55:66"));

    TEST_ASSERT_TRUE(missing.view.contains("Usage: spoof"));
    TEST_ASSERT_TRUE(busy.service.spoofedMacs.empty());
    TEST_ASSERT_TRUE(busy.view.contains("before init Bluetooth"));
    TEST_ASSERT_EQUAL_UINT32(1, success.service.spoofedMacs.size());
    TEST_ASSERT_TRUE(success.view.contains("MAC address overridden"));
}

void test_sniff_switches_to_client_prints_logs_and_stops() {
    BluetoothControllerFixture fixture;
    fixture.utility.currentNowMs = 201;
    fixture.service.sniffLogs = {"ADV packet"};
    fixture.terminalInput.queueReadChar(KEY_NONE);
    fixture.terminalInput.queueReadChar('\n');

    fixture.controller.handleCommand(TerminalCommand("sniff"));

    TEST_ASSERT_EQUAL(BluetoothMode::CLIENT, fixture.service.mode);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.startSniffCalls);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.stopSniffCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("ADV packet"));
    TEST_ASSERT_TRUE(fixture.view.contains("Stopped by user"));
}

void test_reset_and_release_deinitialize_adapter() {
    BluetoothControllerFixture reset;
    reset.service.mode = BluetoothMode::SERVER;
    reset.service.connected = true;
    reset.controller.handleCommand(TerminalCommand("reset"));

    BluetoothControllerFixture release;
    release.controller.ensureConfigured();
    release.controller.ensureReleased();

    TEST_ASSERT_EQUAL_UINT32(1, reset.service.deinitCalls);
    TEST_ASSERT_TRUE(reset.view.contains("Reset complete"));
    TEST_ASSERT_EQUAL_UINT32(1, release.service.initCalls);
    TEST_ASSERT_EQUAL_UINT32(1, release.service.deinitCalls);
}

void test_ensure_configured_warns_when_terminal_is_wifi_hotspot() {
    BluetoothControllerFixture fixture;
    GlobalState::getInstance().setTerminalMode(TerminalTypeEnum::WiFiAp);

    fixture.controller.ensureConfigured();

    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.initCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("Bluetooth is not recommended"));
}

void test_unknown_command_displays_help() {
    BluetoothControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("wat"));

    TEST_ASSERT_TRUE(fixture.view.contains("Unknown command. Available Bluetooth commands"));
}

}  // namespace bluetooth_controller_tests

void runBluetoothControllerTests() {
    using namespace bluetooth_controller_tests;
    RUN_TEST(test_scan_reports_devices_and_empty_results);
    RUN_TEST(test_pair_requires_address_and_reports_discovered_services);
    RUN_TEST(test_pair_reports_connection_failure);
    RUN_TEST(test_status_reports_uninitialized_client_and_server_states);
    RUN_TEST(test_server_starts_default_or_custom_name_and_skips_when_already_connected);
    RUN_TEST(test_keyboard_requires_server_and_sends_decoded_text);
    RUN_TEST(test_keyboard_bridge_forwards_terminal_keys_until_device_input_stops);
    RUN_TEST(test_mouse_click_and_move_require_server_then_delegate);
    RUN_TEST(test_mouse_jiggle_moves_once_then_stops_on_enter);
    RUN_TEST(test_spoof_requires_uninitialized_adapter_and_reports_result);
    RUN_TEST(test_sniff_switches_to_client_prints_logs_and_stops);
    RUN_TEST(test_reset_and_release_deinitialize_adapter);
    RUN_TEST(test_ensure_configured_warns_when_terminal_is_wifi_hotspot);
    RUN_TEST(test_unknown_command_displays_help);
}
