#include <unity.h>

#include "Controllers/EthernetController.h"
#include "../Inputs/FakeInput.h"
#include "../Services/FakeEthernetService.h"
#include "../Services/FakeHttpService.h"
#include "../Services/FakeIcmpService.h"
#include "../Services/FakeNetcatService.h"
#include "../Services/FakeNmapService.h"
#include "../Services/FakeNvsService.h"
#include "../Services/FakeSshService.h"
#include "../Services/FakeTelnetService.h"
#include "../Services/FakeUtilityService.h"
#include "../Services/FakeWifiOpenScannerService.h"
#include "../Services/FakeWifiService.h"
#include "../Shells/FakeModbusShell.h"
#include "../Transformers/FakeJsonTransformer.h"
#include "../Views/FakeDeviceView.h"
#include "../Views/FakeTerminalView.h"

namespace ethernet_controller_tests {

struct EthernetFixture {
    FakeTerminalView view;
    FakeDeviceView device;
    FakeInput terminalInput;
    FakeInput deviceInput;
    FakeUtilityService utility;
    FakeWifiService wifi;
    FakeWifiOpenScannerService wifiOpenScanner;
    FakeEthernetService ethernet;
    FakeSshService ssh;
    FakeNetcatService netcat;
    FakeNmapService nmap;
    FakeIcmpService icmp;
    FakeNvsService nvs;
    FakeHttpService http;
    FakeTelnetService telnet;
    ArgTransformer argTransformer;
    FakeJsonTransformer jsonTransformer;
    UserInputManager userInput{view, terminalInput, argTransformer};
    FakeModbusShell modbusShell;
    HelpShell helpShell{view, terminalInput, userInput};
    EthernetController controller{
        view,
        device,
        terminalInput,
        deviceInput,
        utility,
        wifi,
        wifiOpenScanner,
        ethernet,
        ssh,
        netcat,
        nmap,
        icmp,
        nvs,
        http,
        telnet,
        argTransformer,
        jsonTransformer,
        userInput,
        modbusShell,
        helpShell
    };

    EthernetFixture() {
        GlobalState::getInstance().setCurrentMode(ModeEnum::ETHERNET);
    }
};

void queueDefaultConfig(EthernetFixture& fixture) {
    fixture.terminalInput.queueLine("");
    fixture.terminalInput.queueLine("");
    fixture.terminalInput.queueLine("");
    fixture.terminalInput.queueLine("");
    fixture.terminalInput.queueLine("");
    fixture.terminalInput.queueLine("");
    fixture.terminalInput.queueLine("");
    fixture.terminalInput.queueLine("");
}

void test_config_uses_saved_defaults_and_configures_service() {
    EthernetFixture fixture;
    queueDefaultConfig(fixture);

    fixture.controller.handleCommand(TerminalCommand("config"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.ethernet.configureCalls.size());
    const auto& call = fixture.ethernet.configureCalls[0];
    TEST_ASSERT_EQUAL_INT8(5, call.cs);
    TEST_ASSERT_EQUAL_INT8(-1, call.rst);
    TEST_ASSERT_EQUAL_INT8(18, call.sck);
    TEST_ASSERT_EQUAL_INT8(19, call.miso);
    TEST_ASSERT_EQUAL_INT8(23, call.mosi);
    TEST_ASSERT_EQUAL_INT8(39, call.irq);
    TEST_ASSERT_EQUAL_UINT32(20000000, call.spiHz);
    TEST_ASSERT_EQUAL_UINT8(255, GlobalState::getInstance().getEthernetRstPin());
    TEST_ASSERT_TRUE(fixture.view.contains("W5500 Ethernet configured"));
}

void test_config_reports_failure() {
    EthernetFixture fixture;
    fixture.ethernet.configureResult = false;
    queueDefaultConfig(fixture);

    fixture.controller.handleCommand(TerminalCommand("config"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.ethernet.configureCalls.size());
    TEST_ASSERT_TRUE(fixture.view.contains("configuration failed"));
}

void test_connect_reports_dhcp_success() {
    EthernetFixture fixture;
    fixture.ethernet.beginDhcpResult = true;
    fixture.ethernet.localIp = "10.0.0.50";

    fixture.controller.handleCommand(TerminalCommand("connect"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.ethernet.dhcpTimeouts.size());
    TEST_ASSERT_TRUE(fixture.view.contains("Connected via DHCP"));
    TEST_ASSERT_TRUE(fixture.view.contains("10.0.0.50"));
}

void test_connect_reports_no_link_when_dhcp_fails_without_cable() {
    EthernetFixture fixture;
    fixture.ethernet.beginDhcpResult = false;
    fixture.ethernet.link = false;

    fixture.controller.handleCommand(TerminalCommand("connect"));

    TEST_ASSERT_TRUE(fixture.view.contains("No link"));
}

void test_connect_reports_dhcp_failure_when_link_is_up() {
    EthernetFixture fixture;
    fixture.ethernet.beginDhcpResult = false;
    fixture.ethernet.link = true;

    fixture.controller.handleCommand(TerminalCommand("connect"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.ethernet.dhcpTimeouts.size());
    TEST_ASSERT_TRUE(fixture.view.contains("DHCP failed"));
}

void test_status_prints_connected_network_details() {
    EthernetFixture fixture;
    fixture.ethernet.connected = true;
    fixture.ethernet.link = true;
    fixture.ethernet.localIp = "10.0.0.51";

    fixture.controller.handleCommand(TerminalCommand("status"));

    TEST_ASSERT_TRUE(fixture.view.contains("Link    : UP"));
    TEST_ASSERT_TRUE(fixture.view.contains("IP     : 10.0.0.51"));
    TEST_ASSERT_TRUE(fixture.view.contains("GW     : 10.0.0.1"));
}

void test_status_reports_waiting_for_dhcp_when_link_has_no_ip() {
    EthernetFixture fixture;
    fixture.ethernet.connected = false;
    fixture.ethernet.link = true;
    fixture.ethernet.localIp = "0.0.0.0";

    fixture.controller.handleCommand(TerminalCommand("status"));

    TEST_ASSERT_TRUE(fixture.view.contains("Link    : UP"));
    TEST_ASSERT_TRUE(fixture.view.contains("waiting for DHCP"));
}

void test_status_reports_no_link_ip_state() {
    EthernetFixture fixture;
    fixture.ethernet.connected = false;
    fixture.ethernet.link = false;

    fixture.controller.handleCommand(TerminalCommand("status"));

    TEST_ASSERT_TRUE(fixture.view.contains("Link    : DOWN"));
    TEST_ASSERT_TRUE(fixture.view.contains("no link"));
}

void test_reset_delegates_hard_reset() {
    EthernetFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("reset"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.ethernet.hardResetCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("Interface reset"));
}

void test_ensure_configured_prompts_once_then_reapplies_with_shared_spi() {
    EthernetFixture fixture;
    queueDefaultConfig(fixture);

    fixture.controller.ensureConfigured();
    TEST_ASSERT_EQUAL_UINT32(1, fixture.ethernet.configureCalls.size());

    fixture.ethernet.configureCalls.clear();
    fixture.controller.ensureConfigured();

    TEST_ASSERT_EQUAL_UINT32(1, fixture.ethernet.configureCalls.size());
    TEST_ASSERT_EQUAL_PTR(&fixture.device.spi, fixture.ethernet.configureCalls[0].spi);
    TEST_ASSERT_EQUAL_INT8(5, fixture.ethernet.configureCalls[0].cs);
    TEST_ASSERT_EQUAL_INT8(-1, fixture.ethernet.configureCalls[0].rst);
}

}  // namespace ethernet_controller_tests

void runEthernetControllerTests() {
    using namespace ethernet_controller_tests;
    RUN_TEST(test_config_uses_saved_defaults_and_configures_service);
    RUN_TEST(test_config_reports_failure);
    RUN_TEST(test_connect_reports_dhcp_success);
    RUN_TEST(test_connect_reports_no_link_when_dhcp_fails_without_cable);
    RUN_TEST(test_connect_reports_dhcp_failure_when_link_is_up);
    RUN_TEST(test_status_prints_connected_network_details);
    RUN_TEST(test_status_reports_waiting_for_dhcp_when_link_has_no_ip);
    RUN_TEST(test_status_reports_no_link_ip_state);
    RUN_TEST(test_reset_delegates_hard_reset);
    RUN_TEST(test_ensure_configured_prompts_once_then_reapplies_with_shared_spi);
}
