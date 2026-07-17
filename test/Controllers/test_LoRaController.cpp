#include <unity.h>

#include "Controllers/LoRaController.h"
#include "../Inputs/FakeInput.h"
#include "../Services/FakeI2sService.h"
#include "../Services/FakeLittleFsService.h"
#include "../Services/FakeLoRaService.h"
#include "../Services/FakeUtilityService.h"
#include "../Shells/FakeShell.h"
#include "../Views/FakeDeviceView.h"
#include "../Views/FakeTerminalView.h"

namespace lora_controller_tests {

struct LoRaControllerFixture {
    FakeTerminalView view;
    FakeInput input;
    FakeDeviceView device;
    FakeUtilityService utility;
    FakeLoRaService service;
    FakeLittleFsService littleFs;
    FakeI2sService i2s;
    ArgTransformer argTransformer;
    LoRaTransformer loRaTransformer{utility};
    TerminalCommandTransformer commandTransformer;
    UserInputManager userInput{view, input, argTransformer};
    HelpShell helpShell{view, input, userInput};
    FakeShell meshtasticShell;
    LoRaController controller{
        view,
        input,
        device,
        utility,
        service,
        littleFs,
        i2s,
        argTransformer,
        loRaTransformer,
        commandTransformer,
        userInput,
        helpShell,
        meshtasticShell
    };

    LoRaControllerFixture() {
        auto& state = GlobalState::getInstance();
        state.setCurrentMode(ModeEnum::LORA);
        state.setTerminalMode(TerminalTypeEnum::Standalone);
        state.setLoRaSckPin(5);
        state.setLoRaMisoPin(6);
        state.setLoRaMosiPin(7);
        state.setLoRaCsPin(8);
        state.setLoRaRstPin(9);
        state.setLoRaBusyPin(10);
        state.setLoRaDio1Pin(11);
        state.setLoRaFrequency(868.0f);
        state.setLoRaBandwidth(125);
        state.setLoRaSpreadingFactor(9);
        state.setLoRaCodingRate(7);
        state.setLoRaPower(14);
        state.setLoRaPreambleLength(8);
        state.setLoRaSyncWord(0x1424);
        state.setLoRaCrc(true);
        state.setLoRaInvertIq(false);
        state.setLoRaTcxoVoltage(1.8f);
        state.setI2sBclkPin(1);
        state.setI2sLrckPin(2);
        state.setI2sDataPin(3);
        state.setI2sSampleRate(44100);
        state.setI2sBitsPerSample(16);
        state.setI2sPercentLevel(50);
        service.profile = state.getLoRaProfile();
    }

    void queueDefaultConfig(bool configureRadio = false) {
        for (int i = 0; i < 7; ++i) input.queueLine("");
        input.queueLine(configureRadio ? "y" : "n");
    }

    void configureSuccessfully() {
        queueDefaultConfig(false);
        controller.handleCommand(TerminalCommand("config"));
        view.output.clear();
        view.printlnCalls.clear();
        view.printCalls.clear();
    }
};

void test_config_applies_selected_pins_and_default_radio_profile() {
    LoRaControllerFixture fixture;
    fixture.input.queueLine("12");
    fixture.input.queueLine("13");
    fixture.input.queueLine("14");
    fixture.input.queueLine("15");
    fixture.input.queueLine("16");
    fixture.input.queueLine("17");
    fixture.input.queueLine("18");
    fixture.input.queueLine("n");

    fixture.controller.handleCommand(TerminalCommand("config"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.configurations.size());
    const auto& config = fixture.service.configurations[0];
    TEST_ASSERT_EQUAL_UINT8(12, config.sck);
    TEST_ASSERT_EQUAL_UINT8(13, config.miso);
    TEST_ASSERT_EQUAL_UINT8(14, config.mosi);
    TEST_ASSERT_EQUAL_UINT8(15, config.cs);
    TEST_ASSERT_EQUAL_UINT8(16, config.rst);
    TEST_ASSERT_EQUAL_UINT8(17, config.busy);
    TEST_ASSERT_EQUAL_UINT8(18, config.dio1);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 868.0f, config.profile.frequency);
    TEST_ASSERT_EQUAL_UINT16(125, config.profile.bandwidth);
    TEST_ASSERT_EQUAL_UINT8(9, config.profile.spreadingFactor);
    TEST_ASSERT_TRUE(fixture.view.contains("LoRa SX1262 configured"));
}

void test_config_reports_radio_probe_failure() {
    LoRaControllerFixture fixture;
    fixture.service.configureResult = false;
    fixture.service.lastError = -91;
    fixture.queueDefaultConfig(false);

    fixture.controller.handleCommand(TerminalCommand("config"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.configurations.size());
    TEST_ASSERT_TRUE(fixture.view.contains("LoRa SX1262 setup failed"));
    TEST_ASSERT_TRUE(fixture.view.contains("No valid radio response"));
}

void test_send_hex_payload_delegates_to_service_and_displays_airtime() {
    LoRaControllerFixture fixture;
    fixture.configureSuccessfully();
    fixture.utility.queueNowMs(100);
    fixture.utility.queueNowMs(135);

    fixture.controller.handleCommand(TerminalCommand("send", "hex{", "01 0A FF }"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.transmissions.size());
    const uint8_t expected[] = {0x01, 0x0A, 0xFF};
    TEST_ASSERT_EQUAL_UINT32(sizeof(expected), fixture.service.transmissions[0].payload.size());
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, fixture.service.transmissions[0].payload.data(), sizeof(expected));
    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.airtimeRequests.size());
    TEST_ASSERT_EQUAL_UINT32(3, fixture.service.airtimeRequests[0]);
    TEST_ASSERT_TRUE(fixture.view.contains("LoRa TX: sent"));
    TEST_ASSERT_TRUE(fixture.view.contains("Time: 35 ms"));
}

void test_send_rejects_invalid_hex_without_touching_service() {
    LoRaControllerFixture fixture;
    fixture.configureSuccessfully();

    fixture.controller.handleCommand(TerminalCommand("send", "hex{", "GG }"));

    TEST_ASSERT_TRUE(fixture.service.transmissions.empty());
    TEST_ASSERT_TRUE(fixture.view.contains("LoRa TX: invalid payload"));
}

void test_spam_sends_inline_payload_once_until_enter() {
    LoRaControllerFixture fixture;
    fixture.configureSuccessfully();
    fixture.input.queueReadChar('\0');
    fixture.input.queueReadChar('\n');

    fixture.controller.handleCommand(TerminalCommand("spam", "ping", "25"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.transmissions.size());
    TEST_ASSERT_EQUAL_UINT32(4, fixture.service.transmissions[0].payload.size());
    TEST_ASSERT_EQUAL_UINT8('p', fixture.service.transmissions[0].payload[0]);
    TEST_ASSERT_TRUE(fixture.view.contains("[Spam summary]"));
    TEST_ASSERT_TRUE(fixture.view.contains("Sent: 1"));
}

void test_spam_rejects_invalid_interval_without_transmitting() {
    LoRaControllerFixture fixture;
    fixture.configureSuccessfully();

    fixture.controller.handleCommand(TerminalCommand("spam", "hello", "0"));

    TEST_ASSERT_TRUE(fixture.service.transmissions.empty());
    TEST_ASSERT_TRUE(fixture.view.contains("LoRa spam: invalid interval."));
}

void test_jam_starts_and_stops_continuous_wave() {
    LoRaControllerFixture fixture;
    fixture.configureSuccessfully();
    fixture.input.queueLine("y");
    fixture.input.queueReadChar('\0');
    fixture.input.queueReadChar('\n');

    fixture.controller.handleCommand(TerminalCommand("jam", "1"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.stopContinuousWaveCalls);
    TEST_ASSERT_FALSE(fixture.service.continuousWave);
    TEST_ASSERT_TRUE(fixture.view.contains("[Jam summary]"));
}

void test_jam_can_be_cancelled_before_continuous_wave() {
    LoRaControllerFixture fixture;
    fixture.configureSuccessfully();
    fixture.input.queueLine("n");

    fixture.controller.handleCommand(TerminalCommand("jam", "1"));

    TEST_ASSERT_EQUAL_UINT32(0, fixture.service.stopContinuousWaveCalls);
    TEST_ASSERT_FALSE(fixture.service.continuousWave);
    TEST_ASSERT_TRUE(fixture.view.contains("LoRa jam: cancelled."));
}

void test_receive_reports_packet_errors_and_stops_receiver() {
    LoRaControllerFixture fixture;
    fixture.configureSuccessfully();
    fixture.service.rssi = -64.5f;
    fixture.service.snr = 9.0f;
    fixture.service.pollResults.push_back({ILoRaService::RECEIVE_ERROR, {}});
    fixture.service.pollResults.push_back({ILoRaService::RECEIVE_OK, {0xAA, 0xBB, 0xCC}});
    fixture.input.queueReadChar('\0');
    fixture.input.queueReadChar('\0');
    fixture.input.queueReadChar('\n');

    fixture.controller.handleCommand(TerminalCommand("receive"));

    TEST_ASSERT_FALSE(fixture.service.isReceiving());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.stopReceiveCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("[RX #1]"));
    TEST_ASSERT_TRUE(fixture.view.contains("Length: 3 bytes"));
    TEST_ASSERT_TRUE(fixture.view.contains("Packets: 1"));
    TEST_ASSERT_TRUE(fixture.view.contains("Errors: 1"));
}

void test_record_saves_received_packet_to_littlefs() {
    LoRaControllerFixture fixture;
    fixture.configureSuccessfully();
    fixture.service.rssi = -62.0f;
    fixture.service.snr = 8.5f;
    fixture.service.pollResults.push_back({ILoRaService::RECEIVE_OK, {0xCA, 0xFE}});
    fixture.input.queueReadChar('\0');
    fixture.input.queueLine("capture");

    fixture.controller.handleCommand(TerminalCommand("record"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.stopReceiveCalls);
    TEST_ASSERT_EQUAL_STRING("/capture.lora", fixture.littleFs.lastWritePath.c_str());
    TEST_ASSERT_TRUE(fixture.littleFs.lastWriteData.find("Filetype: ESP32-Bit-Pirate LoRa") != std::string::npos);
    TEST_ASSERT_TRUE(fixture.littleFs.lastWriteData.find("PayloadHex: CA FE") != std::string::npos);
    TEST_ASSERT_TRUE(fixture.view.contains("LoRa record: saved."));
}

void test_record_refuses_when_littlefs_has_not_enough_space() {
    LoRaControllerFixture fixture;
    fixture.configureSuccessfully();
    fixture.littleFs.freeBytesValue = 1024;

    fixture.controller.handleCommand(TerminalCommand("record"));

    TEST_ASSERT_TRUE(fixture.littleFs.lastWritePath.empty());
    TEST_ASSERT_EQUAL_UINT32(0, fixture.service.stopReceiveCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("LoRa record: LittleFS full."));
}

void test_load_sends_selected_lora_file() {
    LoRaControllerFixture fixture;
    fixture.configureSuccessfully();
    LoRaFrame frame;
    frame.profile = GlobalState::getInstance().getLoRaProfile();
    frame.profile.frequency = 915.5f;
    frame.rssi = -50.0f;
    frame.snr = 10.0f;
    frame.payload = {0x01, 0x02, 0x03};
    fixture.littleFs.files["/packet.lora"] =
        fixture.loRaTransformer.transformToFileFormat(frame);
    fixture.input.queueLine("1");

    fixture.controller.handleCommand(TerminalCommand("load"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.transmittedFrames.size());
    TEST_ASSERT_EQUAL_UINT32(3, fixture.service.transmittedFrames[0].payload.size());
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 915.5f, fixture.service.transmittedFrames[0].profile.frequency);
    TEST_ASSERT_TRUE(fixture.view.contains("LoRa load: frame sent."));
}

void test_load_reports_invalid_lora_file_without_transmitting() {
    LoRaControllerFixture fixture;
    fixture.configureSuccessfully();
    fixture.littleFs.files["/bad.lora"] = "not a lora capture";
    fixture.input.queueLine("1");

    fixture.controller.handleCommand(TerminalCommand("load"));

    TEST_ASSERT_TRUE(fixture.service.transmittedFrames.empty());
    TEST_ASSERT_TRUE(fixture.view.contains("LoRa load: invalid .lora file."));
}

void test_rssi_monitor_summarizes_samples() {
    LoRaControllerFixture fixture;
    fixture.configureSuccessfully();
    fixture.service.rssiResults.push_back({-90, -60, -75.5f, 8});
    fixture.service.rssiResults.push_back({-80, -55, -60.0f, 8});
    fixture.input.queueReadChar('\0');
    fixture.input.queueReadChar('\0');
    fixture.input.queueReadChar('\n');

    fixture.controller.handleCommand(TerminalCommand("rssi", "20"));

    TEST_ASSERT_EQUAL_UINT32(2, fixture.service.rssiRequests.size());
    TEST_ASSERT_TRUE(fixture.view.contains("[RSSI #1]"));
    TEST_ASSERT_TRUE(fixture.view.contains("[RSSI #2]"));
    TEST_ASSERT_TRUE(fixture.view.contains("[RSSI summary]"));
    TEST_ASSERT_TRUE(fixture.view.contains("Min: -90 dBm"));
    TEST_ASSERT_TRUE(fixture.view.contains("Max: -55 dBm"));
}

void test_cad_reports_detection_counts() {
    LoRaControllerFixture fixture;
    fixture.configureSuccessfully();
    fixture.service.cadResults.push_back(true);
    fixture.service.cadResults.push_back(false);
    fixture.service.cadResults.push_back(true);
    fixture.input.queueReadChar('\0');
    fixture.input.queueReadChar('\0');
    fixture.input.queueReadChar('\0');
    fixture.input.queueReadChar('\n');

    fixture.controller.handleCommand(TerminalCommand("cad", "20"));

    TEST_ASSERT_TRUE(fixture.view.contains("[Last CAD window]"));
    TEST_ASSERT_TRUE(fixture.view.contains("Checks: 3"));
    TEST_ASSERT_TRUE(fixture.view.contains("Detected: 2"));
}

void test_scan_reports_hit_and_saves_best_frequency() {
    LoRaControllerFixture fixture;
    fixture.configureSuccessfully();
    fixture.input.queueLine("867");
    fixture.input.queueLine("867.2");
    fixture.input.queueLine("0.2");
    fixture.input.queueLine("10");
    fixture.input.queueLine("-80");
    fixture.input.queueReadChar('\0');
    fixture.input.queueReadChar('\n');
    fixture.input.queueLine("y");
    fixture.service.rssiResults.push_back({-95, -70, -82.0f, 4});

    fixture.controller.handleCommand(TerminalCommand("scan"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.rssiRequests.size());
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 867.0f, fixture.service.rssiRequests[0].first);
    TEST_ASSERT_GREATER_OR_EQUAL_UINT32(2, fixture.service.setFrequencies.size());
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 867.0f, GlobalState::getInstance().getLoRaFrequency());
    TEST_ASSERT_TRUE(fixture.view.contains("[HIT]"));
    TEST_ASSERT_TRUE(fixture.view.contains("LoRa frequency saved."));
}

void test_waterfall_draws_one_rssi_sample_and_restores_frequency() {
    LoRaControllerFixture fixture;
    fixture.configureSuccessfully();
    fixture.input.queueLine("867");
    fixture.input.queueLine("867.1");
    fixture.input.queueLine("0.1");
    fixture.input.queueLine("10");
    fixture.input.queueLine("-80");
    fixture.input.queueReadChar('\0');
    fixture.input.queueReadChar('\n');
    fixture.service.rssiResults.push_back({-95, -60, -78.0f, 4});

    fixture.controller.handleCommand(TerminalCommand("waterfall"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.rssiRequests.size());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.device.waterfallCalls.size());
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 867.0f, fixture.device.waterfallCalls[0].minFrequency);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 867.1f, fixture.device.waterfallCalls[0].maxFrequency);
    TEST_ASSERT_EQUAL_STRING("MHz", fixture.device.waterfallCalls[0].unit.c_str());
    TEST_ASSERT_EQUAL_INT(0, fixture.device.waterfallCalls[0].index);
    TEST_ASSERT_GREATER_THAN_INT(1, fixture.device.waterfallCalls[0].count);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 868.0f, fixture.service.setFrequencies.back());
    TEST_ASSERT_TRUE(fixture.view.contains("LoRa waterfall stopped."));
}

void test_ear_maps_rssi_to_i2s_tone_and_releases_audio() {
    LoRaControllerFixture fixture;
    fixture.configureSuccessfully();
    fixture.input.queueLine("-80");
    fixture.input.queueReadChar('\0');
    fixture.input.queueReadChar('\n');
    fixture.service.rssiResults.push_back({-95, -60, -78.0f, 4});

    fixture.controller.handleCommand(TerminalCommand("ear"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.i2s.outputConfigureCalls);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.rssiRequests.size());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.i2s.tones.size());
    TEST_ASSERT_EQUAL_UINT32(15, fixture.i2s.tones[0].durationMs);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.i2s.endCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("LoRa ear: stopped."));
}

void test_set_frequency_saves_value_only_when_service_accepts_it() {
    LoRaControllerFixture fixture;
    fixture.configureSuccessfully();

    fixture.controller.handleCommand(TerminalCommand("setfreq", "915.5"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.setFrequencies.size());
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 915.5f, fixture.service.setFrequencies[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 915.5f, GlobalState::getInstance().getLoRaFrequency());
    TEST_ASSERT_TRUE(fixture.view.contains("LoRa: frequency saved"));
}

void test_set_frequency_reports_service_rejection_without_saving() {
    LoRaControllerFixture fixture;
    fixture.configureSuccessfully();
    fixture.service.setFrequencyResult = false;

    fixture.controller.handleCommand(TerminalCommand("setfreq", "433.92"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.setFrequencies.size());
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 868.0f, GlobalState::getInstance().getLoRaFrequency());
    TEST_ASSERT_TRUE(fixture.view.contains("LoRa: frequency rejected"));
}

void test_status_reports_profile_pins_and_counters() {
    LoRaControllerFixture fixture;
    fixture.configureSuccessfully();
    fixture.service.txPackets = 2;
    fixture.service.txErrors = 1;
    fixture.service.rxPackets = 3;
    fixture.service.rxErrors = 4;
    fixture.service.rxTimeouts = 5;
    fixture.service.rxDropped = 6;
    fixture.service.lastPacketLength = 7;
    fixture.service.rssi = -61.5f;
    fixture.service.snr = 6.25f;

    fixture.controller.handleCommand(TerminalCommand("status"));

    TEST_ASSERT_TRUE(fixture.view.contains("State: ready"));
    TEST_ASSERT_TRUE(fixture.view.contains("Freq: 868.000 MHz"));
    TEST_ASSERT_TRUE(fixture.view.contains("SCK: 5"));
    TEST_ASSERT_TRUE(fixture.view.contains("TX: 2"));
    TEST_ASSERT_TRUE(fixture.view.contains("RX dropped: 6"));
    TEST_ASSERT_TRUE(fixture.view.contains("[Last RX]"));
}

void test_meshtastic_command_delegates_to_shell_after_configuration() {
    LoRaControllerFixture fixture;
    fixture.configureSuccessfully();

    fixture.controller.handleCommand(TerminalCommand("meshtastic"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.meshtasticShell.runCalls);
}

void test_airtime_rejects_out_of_range_lengths() {
    LoRaControllerFixture fixture;
    fixture.configureSuccessfully();

    fixture.controller.handleCommand(TerminalCommand("airtime", "300"));

    TEST_ASSERT_TRUE(fixture.service.airtimeRequests.empty());
    TEST_ASSERT_TRUE(fixture.view.contains("Usage: airtime"));
}

void test_airtime_prints_time_for_valid_length() {
    LoRaControllerFixture fixture;
    fixture.configureSuccessfully();
    fixture.service.defaultAirtimeMs = 321;

    fixture.controller.handleCommand(TerminalCommand("airtime", "12"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.service.airtimeRequests.size());
    TEST_ASSERT_EQUAL_UINT32(12, fixture.service.airtimeRequests[0]);
    TEST_ASSERT_TRUE(fixture.view.contains("[LoRa airtime]"));
    TEST_ASSERT_TRUE(fixture.view.contains("Time: 321 ms"));
}

}  // namespace lora_controller_tests

void runLoRaControllerTests() {
    using namespace lora_controller_tests;
    RUN_TEST(test_config_applies_selected_pins_and_default_radio_profile);
    RUN_TEST(test_config_reports_radio_probe_failure);
    RUN_TEST(test_send_hex_payload_delegates_to_service_and_displays_airtime);
    RUN_TEST(test_send_rejects_invalid_hex_without_touching_service);
    RUN_TEST(test_spam_sends_inline_payload_once_until_enter);
    RUN_TEST(test_spam_rejects_invalid_interval_without_transmitting);
    RUN_TEST(test_jam_starts_and_stops_continuous_wave);
    RUN_TEST(test_jam_can_be_cancelled_before_continuous_wave);
    RUN_TEST(test_receive_reports_packet_errors_and_stops_receiver);
    RUN_TEST(test_record_saves_received_packet_to_littlefs);
    RUN_TEST(test_record_refuses_when_littlefs_has_not_enough_space);
    RUN_TEST(test_load_sends_selected_lora_file);
    RUN_TEST(test_load_reports_invalid_lora_file_without_transmitting);
    RUN_TEST(test_rssi_monitor_summarizes_samples);
    RUN_TEST(test_cad_reports_detection_counts);
    RUN_TEST(test_scan_reports_hit_and_saves_best_frequency);
    RUN_TEST(test_waterfall_draws_one_rssi_sample_and_restores_frequency);
    RUN_TEST(test_ear_maps_rssi_to_i2s_tone_and_releases_audio);
    RUN_TEST(test_set_frequency_saves_value_only_when_service_accepts_it);
    RUN_TEST(test_set_frequency_reports_service_rejection_without_saving);
    RUN_TEST(test_status_reports_profile_pins_and_counters);
    RUN_TEST(test_meshtastic_command_delegates_to_shell_after_configuration);
    RUN_TEST(test_airtime_rejects_out_of_range_lengths);
    RUN_TEST(test_airtime_prints_time_for_valid_length);
}
