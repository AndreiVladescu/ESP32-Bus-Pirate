#include <unity.h>

#include "Abstracts/ANetworkController.h"
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

namespace anetwork_controller_tests {

class NetworkControllerHarness : public ANetworkController {
public:
    using ANetworkController::ANetworkController;

    void ping(const TerminalCommand& cmd) { handlePing(cmd); }
    void discovery(const TerminalCommand& cmd) { handleDiscovery(cmd); }
    void netcat(const TerminalCommand& cmd) { handleNetcat(cmd); }
    void nmap(const TerminalCommand& cmd) { handleNmap(cmd); }
    void ssh(const TerminalCommand& cmd) { handleSsh(cmd); }
    void http(const TerminalCommand& cmd) { handleHttp(cmd); }
    void lookup(const TerminalCommand& cmd) { handleLookup(cmd); }
    void telnet(const TerminalCommand& cmd) { handleTelnet(cmd); }
    void modbus(const TerminalCommand& cmd) { handleModbus(cmd); }
};

struct NetworkFixture {
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
    NetworkControllerHarness controller{
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
};

void test_ping_requires_network_connection() {
    NetworkFixture fixture;

    fixture.controller.ping(TerminalCommand("ping", "example.com"));

    TEST_ASSERT_TRUE(fixture.icmp.pingCalls.empty());
    TEST_ASSERT_TRUE(fixture.view.contains("Ping: You must be connected"));
}

void test_ping_parses_options_and_prints_report() {
    NetworkFixture fixture;
    fixture.wifi.connected = true;
    fixture.wifi.statusRaw = IWifiService::kWifiStatusConnected;
    fixture.icmp.report = "PING ok\n";

    fixture.controller.ping(TerminalCommand("ping", "example.com", "-c 2 -t 300 -i 50"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.icmp.pingCalls.size());
    const auto& call = fixture.icmp.pingCalls[0];
    TEST_ASSERT_EQUAL_STRING("example.com", call.host.c_str());
    TEST_ASSERT_EQUAL_INT(2, call.count);
    TEST_ASSERT_EQUAL_INT(300, call.timeoutMs);
    TEST_ASSERT_EQUAL_INT(50, call.intervalMs);
    TEST_ASSERT_TRUE(fixture.view.contains("PING ok"));
}

void test_ping_help_does_not_start_task() {
    NetworkFixture fixture;
    fixture.ethernet.connected = true;

    fixture.controller.ping(TerminalCommand("ping", "-h"));

    TEST_ASSERT_TRUE(fixture.icmp.pingCalls.empty());
    TEST_ASSERT_TRUE(fixture.view.contains("ping help"));
}

void test_ping_rejects_invalid_count_option() {
    NetworkFixture fixture;
    fixture.wifi.connected = true;

    fixture.controller.ping(TerminalCommand("ping", "example.com", "-c nope"));

    TEST_ASSERT_TRUE(fixture.icmp.pingCalls.empty());
    TEST_ASSERT_TRUE(fixture.view.contains("Invalid count value"));
}

void test_discovery_uses_wifi_ip_clamps_timeout_and_cleans_state() {
    NetworkFixture fixture;
    fixture.wifi.connected = true;
    fixture.wifi.localIp = "192.168.42.99";
    GlobalState::getInstance().setCurrentMode(ModeEnum::WiFi);

    fixture.controller.discovery(TerminalCommand("discovery", "1"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.icmp.discoveryCalls.size());
    TEST_ASSERT_EQUAL_STRING("192.168.42.99", fixture.icmp.discoveryCalls[0].deviceIp.c_str());
    TEST_ASSERT_EQUAL_INT(5, fixture.icmp.discoveryCalls[0].timeoutMs);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.icmp.clearLogCalls);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.icmp.clearDiscoveryCalls);
}

void test_discovery_rejects_invalid_timeout() {
    NetworkFixture fixture;
    fixture.ethernet.connected = true;
    GlobalState::getInstance().setCurrentMode(ModeEnum::ETHERNET);

    fixture.controller.discovery(TerminalCommand("discovery", "slow"));

    TEST_ASSERT_TRUE(fixture.icmp.discoveryCalls.empty());
    TEST_ASSERT_TRUE(fixture.view.contains("Usage: discovery"));
}

void test_discovery_can_be_stopped_by_terminal_input() {
    NetworkFixture fixture;
    fixture.ethernet.connected = true;
    fixture.ethernet.localIp = "10.0.0.8";
    fixture.icmp.discoveryReady = false;
    fixture.icmp.logBatches.push_back({"found 10.0.0.9"});
    GlobalState::getInstance().setCurrentMode(ModeEnum::ETHERNET);

    fixture.controller.discovery(TerminalCommand("discovery", "250"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.icmp.discoveryCalls.size());
    TEST_ASSERT_EQUAL_STRING("10.0.0.8", fixture.icmp.discoveryCalls[0].deviceIp.c_str());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.icmp.stopCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("found 10.0.0.9"));
    TEST_ASSERT_EQUAL_UINT32(1, fixture.icmp.clearLogCalls);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.icmp.clearDiscoveryCalls);
}

void test_netcat_rejects_missing_connection_and_invalid_port() {
    NetworkFixture fixture;

    fixture.controller.netcat(TerminalCommand("nc", "example.com", "23"));
    TEST_ASSERT_TRUE(fixture.netcat.startCalls.empty());
    TEST_ASSERT_TRUE(fixture.view.contains("Netcat: You must be connected"));

    fixture.view.output.clear();
    fixture.wifi.connected = true;
    fixture.controller.netcat(TerminalCommand("nc", "example.com", "bad"));

    TEST_ASSERT_TRUE(fixture.netcat.startCalls.empty());
    TEST_ASSERT_TRUE(fixture.view.contains("Invalid port"));
}

void test_netcat_bridges_input_output_and_closes() {
    NetworkFixture fixture;
    fixture.ethernet.connected = true;
    fixture.netcat.connected = true;
    fixture.netcat.output = "banner";
    fixture.deviceInput.queueReadChar(KEY_NONE);
    fixture.deviceInput.queueReadChar(KEY_OK);
    fixture.terminalInput.queueReadChar('A');

    fixture.controller.netcat(TerminalCommand("nc", "10.0.0.2", "2323"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.netcat.startCalls.size());
    TEST_ASSERT_EQUAL_STRING("10.0.0.2", fixture.netcat.startCalls[0].host.c_str());
    TEST_ASSERT_EQUAL_UINT16(2323, fixture.netcat.startCalls[0].port);
    TEST_ASSERT_TRUE(fixture.netcat.startCalls[0].lineBuffer);
    TEST_ASSERT_FALSE(fixture.netcat.writtenChars.empty());
    TEST_ASSERT_EQUAL_CHAR('A', fixture.netcat.writtenChars[0]);
    TEST_ASSERT_TRUE(fixture.view.contains("banner"));
    TEST_ASSERT_EQUAL_UINT32(1, fixture.netcat.closeCalls);
}

void test_netcat_reports_connection_timeout_and_closes() {
    NetworkFixture fixture;
    fixture.wifi.connected = true;
    fixture.netcat.connected = false;
    fixture.utility.advanceTimeOnSleep = true;

    fixture.controller.netcat(TerminalCommand("nc", "10.0.0.2", "23"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.netcat.startCalls.size());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.netcat.closeCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("Connection failed"));
}

void test_nmap_uses_explicit_ports_udp_and_reports() {
    NetworkFixture fixture;
    fixture.wifi.connected = true;
    fixture.nmap.report = "scan report";

    fixture.controller.nmap(TerminalCommand("nmap", "192.168.1.10", "-p 53,123 -sU -vv"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.nmap.parsedHosts.size());
    TEST_ASSERT_EQUAL_STRING("192.168.1.10", fixture.nmap.parsedHosts[0].c_str());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.nmap.parsedPorts.size());
    TEST_ASSERT_EQUAL_STRING("53,123", fixture.nmap.parsedPorts[0].c_str());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.nmap.layer4Values.size());
    TEST_ASSERT_FALSE(fixture.nmap.layer4Values[0]);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.nmap.startVerbosity.size());
    TEST_ASSERT_EQUAL_INT(2, fixture.nmap.startVerbosity[0]);
    TEST_ASSERT_EQUAL_PTR(&fixture.icmp, fixture.nmap.icmpService);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.nmap.cleanCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("scan report"));
    TEST_ASSERT_TRUE(fixture.view.contains("Scan finished"));
}

void test_nmap_rejects_invalid_host_and_invalid_port_spec() {
    NetworkFixture fixture;
    fixture.ethernet.connected = true;
    fixture.nmap.parseHostsResult = false;

    fixture.controller.nmap(TerminalCommand("nmap", "bad host"));

    TEST_ASSERT_TRUE(fixture.nmap.startVerbosity.empty());
    TEST_ASSERT_TRUE(fixture.view.contains("Invalid host"));

    fixture.view.output.clear();
    fixture.nmap.parseHostsResult = true;
    fixture.nmap.parsePortsResult = false;
    fixture.controller.nmap(TerminalCommand("nmap", "192.168.1.10", "-p nope"));

    TEST_ASSERT_TRUE(fixture.nmap.startVerbosity.empty());
    TEST_ASSERT_TRUE(fixture.view.contains("invalid -p value"));
}

void test_nmap_without_ports_uses_default_tcp_ports() {
    NetworkFixture fixture;
    fixture.wifi.connected = true;

    fixture.controller.nmap(TerminalCommand("nmap", "example.com", "-v"));

    TEST_ASSERT_TRUE(fixture.nmap.parsedPorts.empty());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.nmap.defaultPortsTcp.size());
    TEST_ASSERT_TRUE(fixture.nmap.defaultPortsTcp[0]);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.nmap.startVerbosity.size());
    TEST_ASSERT_EQUAL_INT(1, fixture.nmap.startVerbosity[0]);
    TEST_ASSERT_TRUE(fixture.view.contains("Using top 100 common ports"));
}

void test_ssh_rejects_invalid_port_without_starting() {
    NetworkFixture fixture;
    fixture.wifi.connected = true;

    fixture.controller.ssh(TerminalCommand("ssh", "10.0.0.5", "root pass nope"));

    TEST_ASSERT_TRUE(fixture.ssh.startCalls.empty());
    TEST_ASSERT_TRUE(fixture.view.contains("SSH: Invalid port"));
}

void test_ssh_bridges_terminal_output_and_closes() {
    NetworkFixture fixture;
    fixture.ethernet.connected = true;
    fixture.ssh.connected = true;
    fixture.ssh.output = "shell>";
    fixture.terminalInput.queueReadChar('l');
    fixture.deviceInput.queueReadChar(KEY_NONE);
    fixture.deviceInput.queueReadChar(KEY_OK);

    fixture.controller.ssh(TerminalCommand("ssh", "10.0.0.5", "root pass 2222"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.ssh.startCalls.size());
    TEST_ASSERT_EQUAL_STRING("10.0.0.5", fixture.ssh.startCalls[0].host.c_str());
    TEST_ASSERT_EQUAL_STRING("root", fixture.ssh.startCalls[0].user.c_str());
    TEST_ASSERT_EQUAL_STRING("pass", fixture.ssh.startCalls[0].pass.c_str());
    TEST_ASSERT_EQUAL_INT(2222, fixture.ssh.startCalls[0].port);
    TEST_ASSERT_FALSE(fixture.ssh.writtenChars.empty());
    TEST_ASSERT_EQUAL_CHAR('l', fixture.ssh.writtenChars[0]);
    TEST_ASSERT_TRUE(fixture.view.contains("shell>"));
    TEST_ASSERT_EQUAL_UINT32(1, fixture.ssh.closeCalls);
}

void test_ssh_reports_connection_timeout_and_closes() {
    NetworkFixture fixture;
    fixture.wifi.connected = true;
    fixture.ssh.connected = false;
    fixture.utility.advanceTimeOnSleep = true;

    fixture.controller.ssh(TerminalCommand("ssh", "10.0.0.5", "root pass 22"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.ssh.startCalls.size());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.ssh.closeCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("Connection failed"));
}

void test_http_get_normalizes_url_prints_response_and_resets_service() {
    NetworkFixture fixture;
    fixture.ethernet.connected = true;
    fixture.http.response = "HTTP/1.1 200 OK\r\n\r\nhello";

    fixture.controller.http(TerminalCommand("http", "get", "example.com"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.http.getCalls.size());
    TEST_ASSERT_EQUAL_STRING("https://example.com", fixture.http.getCalls[0].url.c_str());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.http.resetCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("HTTP GET"));
    TEST_ASSERT_TRUE(fixture.view.contains("hello"));
}

void test_http_rejects_unsupported_method_without_request() {
    NetworkFixture fixture;
    fixture.wifi.connected = true;

    fixture.controller.http(TerminalCommand("http", "post", "https://example.com"));

    TEST_ASSERT_TRUE(fixture.http.getCalls.empty());
    TEST_ASSERT_TRUE(fixture.http.fetchCalls.empty());
    TEST_ASSERT_TRUE(fixture.view.contains("Only GET implemented"));
}

void test_http_get_reports_timeout_and_resets_service() {
    NetworkFixture fixture;
    fixture.ethernet.connected = true;
    fixture.http.responseReady = false;
    fixture.utility.advanceTimeOnSleep = true;

    fixture.controller.http(TerminalCommand("http", "get", "example.com"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.http.getCalls.size());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.http.resetCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("request timed out"));
}

void test_http_analyze_fetches_public_sources_without_optional_w3c() {
    NetworkFixture fixture;
    fixture.wifi.connected = true;
    fixture.http.fetchResponse = "{\"source\":\"ok\"}";
    fixture.jsonTransformer.lines = {"source: ok"};
    fixture.terminalInput.queueLine("n");

    fixture.controller.http(TerminalCommand("http", "analyze", "example.com"));

    TEST_ASSERT_EQUAL_UINT32(2, fixture.http.fetchCalls.size());
    TEST_ASSERT_TRUE(fixture.http.fetchCalls[0].url.find("urlscan.io") != std::string::npos);
    TEST_ASSERT_TRUE(fixture.http.fetchCalls[0].url.find("page.domain:example.com") != std::string::npos);
    TEST_ASSERT_TRUE(fixture.http.fetchCalls[1].url.find("ssllabs.com") != std::string::npos);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.http.resetCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("source: ok"));
    TEST_ASSERT_TRUE(fixture.view.contains("HTTP Analyze: Finished"));
}

void test_lookup_mac_fetches_json_and_prints_transformed_lines() {
    NetworkFixture fixture;
    fixture.wifi.connected = true;
    fixture.jsonTransformer.lines = {"vendor: BusPirate"};

    fixture.controller.lookup(TerminalCommand("lookup", "mac", "AA:BB:CC:DD:EE:FF"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.http.fetchCalls.size());
    TEST_ASSERT_TRUE(fixture.http.fetchCalls[0].url.find("api.maclookup.app") != std::string::npos);
    TEST_ASSERT_TRUE(fixture.view.contains("vendor: BusPirate"));
    TEST_ASSERT_EQUAL_UINT32(1, fixture.http.resetCalls);
}

void test_lookup_ip_fetches_both_sources_and_prints_transformed_lines() {
    NetworkFixture fixture;
    fixture.ethernet.connected = true;
    fixture.jsonTransformer.lines = {"ip: 1.2.3.4"};

    fixture.controller.lookup(TerminalCommand("lookup", "ip", "1.2.3.4"));

    TEST_ASSERT_EQUAL_UINT32(2, fixture.http.fetchCalls.size());
    TEST_ASSERT_EQUAL_STRING("http://ip-api.com/json/1.2.3.4", fixture.http.fetchCalls[0].url.c_str());
    TEST_ASSERT_TRUE(fixture.http.fetchCalls[1].url.find("isc.sans.edu/api/ip/1.2.3.4") != std::string::npos);
    TEST_ASSERT_TRUE(fixture.view.contains("ip: 1.2.3.4"));
    TEST_ASSERT_EQUAL_UINT32(1, fixture.http.resetCalls);
}

void test_telnet_connects_bridges_once_and_closes() {
    NetworkFixture fixture;
    fixture.ethernet.connected = true;
    fixture.terminalInput.queueReadChar('x');
    fixture.deviceInput.queueReadChar(KEY_OK);

    fixture.controller.telnet(TerminalCommand("telnet", "10.0.0.5", "2323"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.telnet.connectCalls.size());
    TEST_ASSERT_EQUAL_STRING("10.0.0.5", fixture.telnet.connectCalls[0].host.c_str());
    TEST_ASSERT_EQUAL_UINT16(2323, fixture.telnet.connectCalls[0].port);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.telnet.writtenChars.size());
    TEST_ASSERT_EQUAL_CHAR('x', fixture.telnet.writtenChars[0]);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.telnet.closeCalls);
}

void test_telnet_reports_connection_failure() {
    NetworkFixture fixture;
    fixture.wifi.connected = true;
    fixture.telnet.connectResult = false;
    fixture.telnet.error = "refused";

    fixture.controller.telnet(TerminalCommand("telnet", "10.0.0.5"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.telnet.connectCalls.size());
    TEST_ASSERT_EQUAL_UINT16(23, fixture.telnet.connectCalls[0].port);
    TEST_ASSERT_TRUE(fixture.view.contains("Connection failed: refused"));
    TEST_ASSERT_EQUAL_UINT32(0, fixture.telnet.closeCalls);
}

void test_modbus_delegates_to_shell_with_default_port() {
    NetworkFixture fixture;
    fixture.wifi.connected = true;

    fixture.controller.modbus(TerminalCommand("modbus", "192.168.1.10"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.modbusShell.runCalls.size());
    TEST_ASSERT_EQUAL_STRING("192.168.1.10", fixture.modbusShell.runCalls[0].host.c_str());
    TEST_ASSERT_EQUAL_UINT16(502, fixture.modbusShell.runCalls[0].port);
}

void test_modbus_rejects_missing_connection_and_accepts_custom_port() {
    NetworkFixture fixture;

    fixture.controller.modbus(TerminalCommand("modbus", "192.168.1.10"));
    TEST_ASSERT_TRUE(fixture.modbusShell.runCalls.empty());
    TEST_ASSERT_TRUE(fixture.view.contains("Modbus: You must be connected"));

    fixture.view.output.clear();
    fixture.ethernet.connected = true;
    fixture.controller.modbus(TerminalCommand("modbus", "192.168.1.10", "1502"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.modbusShell.runCalls.size());
    TEST_ASSERT_EQUAL_UINT16(1502, fixture.modbusShell.runCalls[0].port);
}

}  // namespace anetwork_controller_tests

void runANetworkControllerTests() {
    using namespace anetwork_controller_tests;
    RUN_TEST(test_ping_requires_network_connection);
    RUN_TEST(test_ping_parses_options_and_prints_report);
    RUN_TEST(test_ping_help_does_not_start_task);
    RUN_TEST(test_ping_rejects_invalid_count_option);
    RUN_TEST(test_discovery_uses_wifi_ip_clamps_timeout_and_cleans_state);
    RUN_TEST(test_discovery_rejects_invalid_timeout);
    RUN_TEST(test_discovery_can_be_stopped_by_terminal_input);
    RUN_TEST(test_netcat_rejects_missing_connection_and_invalid_port);
    RUN_TEST(test_netcat_bridges_input_output_and_closes);
    RUN_TEST(test_netcat_reports_connection_timeout_and_closes);
    RUN_TEST(test_nmap_uses_explicit_ports_udp_and_reports);
    RUN_TEST(test_nmap_rejects_invalid_host_and_invalid_port_spec);
    RUN_TEST(test_nmap_without_ports_uses_default_tcp_ports);
    RUN_TEST(test_ssh_rejects_invalid_port_without_starting);
    RUN_TEST(test_ssh_bridges_terminal_output_and_closes);
    RUN_TEST(test_ssh_reports_connection_timeout_and_closes);
    RUN_TEST(test_http_get_normalizes_url_prints_response_and_resets_service);
    RUN_TEST(test_http_rejects_unsupported_method_without_request);
    RUN_TEST(test_http_get_reports_timeout_and_resets_service);
    RUN_TEST(test_http_analyze_fetches_public_sources_without_optional_w3c);
    RUN_TEST(test_lookup_mac_fetches_json_and_prints_transformed_lines);
    RUN_TEST(test_lookup_ip_fetches_both_sources_and_prints_transformed_lines);
    RUN_TEST(test_telnet_connects_bridges_once_and_closes);
    RUN_TEST(test_telnet_reports_connection_failure);
    RUN_TEST(test_modbus_delegates_to_shell_with_default_port);
    RUN_TEST(test_modbus_rejects_missing_connection_and_accepts_custom_port);
}
