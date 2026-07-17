#include <unity.h>

#include <string>
#include <vector>

#include "Controllers/SubGhzController.h"
#include "../Inputs/FakeInput.h"
#include "../Services/FakeI2sService.h"
#include "../Services/FakeLittleFsService.h"
#include "../Services/FakePinService.h"
#include "../Services/FakeSubGhzService.h"
#include "../Services/FakeUtilityService.h"
#include "../Views/FakeDeviceView.h"
#include "../Views/FakeTerminalView.h"

namespace subghz_controller_tests {

rmt_symbol_word_t symbol(uint32_t duration0, uint32_t level0, uint32_t duration1, uint32_t level1) {
    rmt_symbol_word_t s{};
    s.duration0 = duration0;
    s.level0 = level0;
    s.duration1 = duration1;
    s.level1 = level1;
    return s;
}

std::vector<rmt_symbol_word_t> pulseLengthFrame() {
    std::vector<rmt_symbol_word_t> frame;
    const std::string bits = "101010101010";
    frame.reserve(bits.size());
    for (char bit : bits) {
        if (bit == '1') frame.push_back(symbol(600, 1, 200, 0));
        else frame.push_back(symbol(200, 1, 600, 0));
    }
    return frame;
}

std::vector<rmt_symbol_word_t> replayableRawFrame() {
    return {
        symbol(100, 1, 200, 0),
        symbol(300, 1, 100, 0),
        symbol(100, 1, 200, 0),
        symbol(300, 1, 100, 0),
        symbol(100, 1, 200, 0),
        symbol(300, 1, 100, 0),
        symbol(100, 1, 200, 0)
    };
}

struct SubGhzControllerFixture {
    FakeTerminalView view;
    FakeInput input;
    FakeDeviceView device;
    FakeUtilityService utility;
    FakeSubGhzService subGhzService;
    FakePinService pinService;
    FakeI2sService i2sService;
    FakeLittleFsService littleFsService;
    ArgTransformer argTransformer;
    SubGhzTransformer subGhzTransformer;
    SubGhzAnalyzer subGhzAnalyzer;
    UserInputManager userInput{view, input, argTransformer};
    HelpShell helpShell{view, input, userInput};
    SubGhzController controller{
        view,
        input,
        device,
        utility,
        subGhzService,
        pinService,
        i2sService,
        littleFsService,
        argTransformer,
        subGhzTransformer,
        userInput,
        subGhzAnalyzer,
        helpShell
    };

    SubGhzControllerFixture() {
        GlobalState::getInstance().setCurrentMode(ModeEnum::SUBGHZ);
    }
};

void queueDefaultConfiguration(SubGhzControllerFixture& fixture) {
    fixture.input.queueLine("");
    fixture.input.queueLine("");
    fixture.input.queueLine("");
    fixture.input.queueLine("");
    fixture.input.queueLine("");
}

void test_config_updates_state_and_applies_scan_profile() {
    SubGhzControllerFixture fixture;
    fixture.input.queueLine("20");
    fixture.input.queueLine("21");
    fixture.input.queueLine("22");
    fixture.input.queueLine("23");
    fixture.input.queueLine("24");

    fixture.controller.handleCommand(TerminalCommand("config"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.configureCalls.size());
    const auto& call = fixture.subGhzService.configureCalls[0];
    TEST_ASSERT_EQUAL_UINT8(20, call.sck);
    TEST_ASSERT_EQUAL_UINT8(21, call.miso);
    TEST_ASSERT_EQUAL_UINT8(22, call.mosi);
    TEST_ASSERT_EQUAL_UINT8(23, call.ss);
    TEST_ASSERT_EQUAL_UINT8(24, call.gdo0);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 433.92f, call.mhz);
    TEST_ASSERT_EQUAL_UINT8(20, GlobalState::getInstance().getSubGhzSckPin());
    TEST_ASSERT_EQUAL_UINT8(24, GlobalState::getInstance().getSubGhzGdoPin());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.tuneCalls.size());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.scanProfileDataRates.size());
    TEST_ASSERT_TRUE(fixture.view.contains("CC1101 module detected and configured"));
}

void test_config_reports_probe_failure_without_applying_profile() {
    SubGhzControllerFixture fixture;
    fixture.subGhzService.configureResult = false;
    queueDefaultConfiguration(fixture);

    fixture.controller.handleCommand(TerminalCommand("config"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.configureCalls.size());
    TEST_ASSERT_TRUE(fixture.subGhzService.tuneCalls.empty());
    TEST_ASSERT_TRUE(fixture.subGhzService.scanProfileDataRates.empty());
    TEST_ASSERT_TRUE(fixture.view.contains("Failed to detect CC1101"));
}

void test_ensure_configured_prompts_once_after_success() {
    SubGhzControllerFixture fixture;
    queueDefaultConfiguration(fixture);

    fixture.controller.ensureConfigured();
    fixture.controller.ensureConfigured();

    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.configureCalls.size());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.scanProfileDataRates.size());
}

void test_setfreq_custom_updates_state_and_tunes_radio() {
    SubGhzControllerFixture fixture;
    fixture.input.queueLine("");
    fixture.input.queueLine("315.25");

    fixture.controller.handleCommand(TerminalCommand("setfreq"));

    TEST_ASSERT_FLOAT_WITHIN(0.001f, 315.25f, GlobalState::getInstance().getSubGhzFrequency());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.tuneCalls.size());
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 315.25f, fixture.subGhzService.tuneCalls[0]);
    TEST_ASSERT_TRUE(fixture.view.contains("Frequency changed to 315.25 MHz"));
}

void test_send_builds_princeton_command_from_numeric_payload() {
    SubGhzControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("send", "0xABCDEF", "500"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.sentCommands.size());
    const auto& sent = fixture.subGhzService.sentCommands[0];
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SubGhzProtocolEnum::Princeton), static_cast<int>(sent.protocol));
    TEST_ASSERT_EQUAL_HEX64(0xABCDEFULL, sent.key);
    TEST_ASSERT_EQUAL_UINT16(24, sent.bits);
    TEST_ASSERT_EQUAL_UINT16(500, sent.te_us);
    TEST_ASSERT_EQUAL_UINT32(433920000, sent.frequency_hz);
    TEST_ASSERT_TRUE(fixture.view.contains("SUBGHZ: Sent"));
}

void test_send_rejects_missing_payload_without_touching_service() {
    SubGhzControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("send"));

    TEST_ASSERT_TRUE(fixture.subGhzService.sentCommands.empty());
    TEST_ASSERT_TRUE(fixture.view.contains("Usage: send <payload> [te]"));
}

void test_raw_prints_pulses_and_stops_sniffer() {
    SubGhzControllerFixture fixture;
    fixture.subGhzService.rawPulses.push_back({"RAW: 100 -200 300", 9});
    fixture.input.queueReadChar(KEY_NONE);
    fixture.input.queueReadChar('\n');

    fixture.controller.handleCommand(TerminalCommand("raw"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.sniffProfiles.size());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.rawSnifferPins.size());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.stopRawSnifferCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("RAW: 100 -200 300"));
    TEST_ASSERT_TRUE(fixture.view.contains("9 pulses"));
}

void test_scan_detects_peak_and_saves_best_frequency() {
    SubGhzControllerFixture fixture;
    fixture.input.queueLine("");
    fixture.input.queueLine("4");
    fixture.input.queueLine("-67");
    fixture.input.queueReadChar(KEY_NONE);
    fixture.input.queueReadChar('\n');
    fixture.input.queueLine("y");
    fixture.subGhzService.rssiSamples.push_back(-60);

    fixture.controller.handleCommand(TerminalCommand("scan"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.scanBands.size());
    TEST_ASSERT_EQUAL_STRING("All", fixture.subGhzService.scanBands[0].c_str());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.scanProfileDataRates.size());
    TEST_ASSERT_GREATER_OR_EQUAL_UINT32(2, fixture.subGhzService.tuneCalls.size());
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 315.0f, fixture.subGhzService.tuneCalls.front());
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 315.0f, fixture.subGhzService.tuneCalls.back());
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 315.0f, GlobalState::getInstance().getSubGhzFrequency());
    TEST_ASSERT_TRUE(fixture.view.contains("[PEAK] f=315.00 MHz"));
    TEST_ASSERT_TRUE(fixture.view.contains("[FREQ] Saving to config: 315.00 MHz"));
}

void test_scan_reports_missing_radio_without_tuning() {
    SubGhzControllerFixture fixture;
    fixture.subGhzService.applyScanProfileResult = false;
    fixture.input.queueLine("");
    fixture.input.queueLine("");
    fixture.input.queueLine("");

    fixture.controller.handleCommand(TerminalCommand("scan"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.scanProfileDataRates.size());
    TEST_ASSERT_TRUE(fixture.subGhzService.tuneCalls.empty());
    TEST_ASSERT_TRUE(fixture.view.contains("SUBGHZ: Not detected. Run 'config' first."));
}

void test_sweep_analyzes_one_frequency_and_stops() {
    SubGhzControllerFixture fixture;
    fixture.input.queueLine("");
    fixture.input.queueLine("20");
    fixture.input.queueLine("20");
    fixture.input.queueLine("-70");
    fixture.input.queueReadChar(KEY_NONE);
    fixture.input.queueReadChar(KEY_NONE);
    fixture.input.queueReadChar('\n');
    fixture.subGhzService.rssiSamples.push_back(-50);

    fixture.controller.handleCommand(TerminalCommand("sweep"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.scanBands.size());
    TEST_ASSERT_EQUAL_STRING("All", fixture.subGhzService.scanBands[0].c_str());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.scanProfileDataRates.size());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.tuneCalls.size());
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 315.0f, fixture.subGhzService.tuneCalls[0]);
    TEST_ASSERT_TRUE(fixture.view.contains("315.00 MHz  peak=-50 dBm"));
    TEST_ASSERT_TRUE(fixture.view.contains("SUBGHZ Sweep: Stopped by user."));
}

void test_decode_analyzes_received_frame_and_stops_sniffer() {
    SubGhzControllerFixture fixture;
    fixture.subGhzService.rawFrames.push_back(pulseLengthFrame());
    fixture.input.queueReadChar(KEY_NONE);
    fixture.input.queueReadChar('\n');

    fixture.controller.handleCommand(TerminalCommand("decode"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.sniffProfiles.size());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.rawSnifferPins.size());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.stopRawSnifferCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("Encoding     : PulseLength"));
    TEST_ASSERT_TRUE(fixture.view.contains("Payload      : AAA"));
}

void test_receive_routes_to_decode_by_default() {
    SubGhzControllerFixture fixture;
    fixture.input.queueLine("");
    fixture.input.queueReadChar(KEY_NONE);
    fixture.input.queueReadChar('\n');
    fixture.subGhzService.rawFrames.push_back(pulseLengthFrame());

    fixture.controller.handleCommand(TerminalCommand("receive"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.sniffProfiles.size());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.rawSnifferPins.size());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.stopRawSnifferCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("SUBGHZ Decode: Listening"));
    TEST_ASSERT_TRUE(fixture.view.contains("Encoding     : PulseLength"));
}

void test_receive_routes_to_raw_when_decode_is_declined() {
    SubGhzControllerFixture fixture;
    fixture.input.queueLine("n");
    fixture.input.queueReadChar(KEY_NONE);
    fixture.input.queueReadChar('\n');
    fixture.subGhzService.rawPulses.push_back({"RAW: 12 -34 56", 10});

    fixture.controller.handleCommand(TerminalCommand("receive"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.sniffProfiles.size());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.rawSnifferPins.size());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.stopRawSnifferCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("SUBGHZ Raw: Frequency"));
    TEST_ASSERT_TRUE(fixture.view.contains("RAW: 12 -34 56"));
}

void test_replay_raw_captures_and_sends_once() {
    SubGhzControllerFixture fixture;
    fixture.input.queueLine("y");
    fixture.input.queueLine("y");
    fixture.input.queueLine("n");
    fixture.subGhzService.rawSymbolsUntilFrames.push_back(replayableRawFrame());

    fixture.controller.handleCommand(TerminalCommand("replay"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.sniffProfiles.size());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.rawSnifferPins.size());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.stopRawSnifferCalls);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.rawSendProfiles.size());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.sendRawFrameCalls.size());
    TEST_ASSERT_EQUAL_UINT32(7, fixture.subGhzService.sendRawFrameCalls[0].items.size());
    TEST_ASSERT_TRUE(fixture.view.contains("Captured 7 symbols."));
    TEST_ASSERT_TRUE(fixture.view.contains("RAW window sent."));
}

void test_replay_raw_can_be_cancelled_before_capture() {
    SubGhzControllerFixture fixture;
    fixture.input.queueLine("n");

    fixture.controller.handleCommand(TerminalCommand("replay"));

    TEST_ASSERT_TRUE(fixture.subGhzService.sniffProfiles.empty());
    TEST_ASSERT_TRUE(fixture.subGhzService.rawSnifferPins.empty());
    TEST_ASSERT_TRUE(fixture.subGhzService.sendRawFrameCalls.empty());
}

void test_jam_single_frequency_toggles_gdo_until_enter() {
    SubGhzControllerFixture fixture;
    fixture.input.queueLine("y");
    fixture.input.queueLine("n");
    fixture.input.queueLine("");
    fixture.input.queueReadChar(KEY_NONE);
    fixture.input.queueReadChar('\n');

    fixture.controller.handleCommand(TerminalCommand("jam"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.rawSendProfiles.size());
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 433.92f, fixture.subGhzService.rawSendProfiles[0]);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.startTxBitBangCalls);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.stopTxBitBangCalls);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.deinitRfModuleCalls);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.pinService.highCalls.size());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.pinService.lowCalls.size());
    TEST_ASSERT_TRUE(fixture.view.contains("SUBGHZ Jam: Stopped by user."));
}

void test_jam_can_be_cancelled_before_transmit() {
    SubGhzControllerFixture fixture;
    fixture.input.queueLine("n");

    fixture.controller.handleCommand(TerminalCommand("jam"));

    TEST_ASSERT_TRUE(fixture.subGhzService.rawSendProfiles.empty());
    TEST_ASSERT_EQUAL_UINT32(0, fixture.subGhzService.startTxBitBangCalls);
    TEST_ASSERT_TRUE(fixture.pinService.highCalls.empty());
}

void test_bruteforce_sends_first_code_then_stops() {
    SubGhzControllerFixture fixture;
    fixture.input.queueLine("1");
    fixture.input.queueLine("");
    fixture.input.queueLine("1");
    fixture.input.queueReadChar('\n');

    fixture.controller.handleCommand(TerminalCommand("bruteforce"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.rawSendProfiles.size());
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 433.92f, fixture.subGhzService.rawSendProfiles[0]);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.startTxBitBangCalls);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.stopTxBitBangCalls);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.deinitRfModuleCalls);
    TEST_ASSERT_FALSE(fixture.subGhzService.rawPulsePins.empty());
    TEST_ASSERT_TRUE(fixture.view.contains("SUBGHZ BruteForce: Stopped by user."));
}

void test_trace_draws_logic_trace_then_stops() {
    SubGhzControllerFixture fixture;
    fixture.pinService.defaultRead = true;
    fixture.input.queueReadChar('\n');
    fixture.utility.queueNowMs(0);
    for (int i = 0; i < 240; ++i) {
        fixture.utility.queueNowMs(0);
    }
    fixture.utility.queueNowMs(10);
    fixture.utility.queueNowMs(10);

    fixture.controller.handleCommand(TerminalCommand("trace"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.sniffProfiles.size());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.device.logicTraceCalls.size());
    TEST_ASSERT_EQUAL_UINT32(240, fixture.device.logicTraceCalls[0].samples.size());
    TEST_ASSERT_EQUAL_UINT8(1, fixture.device.logicTraceCalls[0].scale);
    TEST_ASSERT_EQUAL_UINT32(240, fixture.pinService.readCalls.size());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.device.topBarTitles.size());
    TEST_ASSERT_TRUE(fixture.view.contains("SUBGHZ Trace: Stopped by user."));
}

void test_waterfall_draws_one_frequency_and_restores_previous_frequency() {
    SubGhzControllerFixture fixture;
    fixture.input.queueLine("");
    fixture.input.queueLine("");
    fixture.input.queueReadChar(KEY_NONE);
    fixture.input.queueReadChar('\n');
    fixture.subGhzService.rssiSamples.push_back(-45);

    fixture.controller.handleCommand(TerminalCommand("waterfall"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.scanBands.size());
    TEST_ASSERT_EQUAL_STRING("All", fixture.subGhzService.scanBands[0].c_str());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.scanProfileDataRates.size());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.device.waterfallCalls.size());
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 315.0f, fixture.subGhzService.tuneCalls.front());
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 433.92f, fixture.subGhzService.tuneCalls.back());
    TEST_ASSERT_EQUAL_STRING("MHz", fixture.device.waterfallCalls[0].unit.c_str());
    TEST_ASSERT_EQUAL_INT(0, fixture.device.waterfallCalls[0].index);
    TEST_ASSERT_EQUAL_INT(2, fixture.device.waterfallCalls[0].count);
    TEST_ASSERT_TRUE(fixture.device.waterfallCalls[0].level > 0);
    TEST_ASSERT_TRUE(fixture.view.contains("SUBGHZ Waterfall: Stopped by user."));
}

void test_ear_maps_rssi_to_i2s_tone_until_enter() {
    SubGhzControllerFixture fixture;
    fixture.input.queueLine("");
    fixture.input.queueLine("-70");
    fixture.input.queueReadChar(KEY_NONE);
    fixture.input.queueReadChar('\n');
    fixture.subGhzService.rssiSamples.push_back(-50);

    fixture.controller.handleCommand(TerminalCommand("ear"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.sniffProfiles.size());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.tuneCalls.size());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.i2sService.outputConfigureCalls);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.i2sService.tones.size());
    TEST_ASSERT_TRUE(fixture.i2sService.tones[0].frequency > 800);
    TEST_ASSERT_TRUE(fixture.i2sService.tones[0].frequency < 12000);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.i2sService.tones[0].durationMs);
    TEST_ASSERT_TRUE(fixture.view.contains("SUBGHZ Ear: Stopped by user."));
}

void test_load_sends_selected_sub_file_command() {
    SubGhzControllerFixture fixture;
    fixture.littleFsService.files["/gate.sub"] =
        "Filetype: Flipper SubGhz Key File\n"
        "Version: 1\n"
        "Frequency: 433920000\n"
        "Preset: FuriHalSubGhzPresetOok650Async\n"
        "Protocol: Princeton\n"
        "TE: 350\n"
        "Bit: 24\n"
        "Key: 00 00 00 00 00 AB CD EF\n";
    fixture.input.queueLine("1");
    fixture.input.queueLine("1");
    fixture.input.queueLine("2");

    fixture.controller.handleCommand(TerminalCommand("load"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.sentCommands.size());
    TEST_ASSERT_EQUAL_HEX64(0xABCDEFULL, fixture.subGhzService.sentCommands[0].key);
    TEST_ASSERT_TRUE(fixture.view.contains("Commands in file 'gate.sub'"));
    TEST_ASSERT_TRUE(fixture.view.contains("Princeton"));
}

void test_load_reports_missing_sub_files() {
    SubGhzControllerFixture fixture;
    fixture.littleFsService.files.clear();

    fixture.controller.handleCommand(TerminalCommand("load"));

    TEST_ASSERT_TRUE(fixture.subGhzService.sentCommands.empty());
    TEST_ASSERT_TRUE(fixture.view.contains("No .sub files found"));
}

void test_record_saves_raw_capture_as_sub_file() {
    SubGhzControllerFixture fixture;
    fixture.utility.currentNowMs = 12345;
    fixture.subGhzService.rawSymbolsUntilFrames.push_back({
        symbol(100, 1, 200, 0),
        symbol(300, 1, 100, 0),
        symbol(100, 1, 200, 0),
        symbol(300, 1, 100, 0),
        symbol(100, 1, 200, 0),
        symbol(300, 1, 100, 0),
        symbol(100, 1, 200, 0)
    });
    fixture.input.queueLine("");
    fixture.input.queueReadChar(KEY_NONE);
    fixture.input.queueLine("");
    fixture.input.queueLine("capture");

    fixture.controller.handleCommand(TerminalCommand("record"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.sniffProfiles.size());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.rawSnifferPins.size());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.subGhzService.stopRawSnifferCalls);
    TEST_ASSERT_EQUAL_STRING("/capture.sub", fixture.littleFsService.lastWritePath.c_str());
    TEST_ASSERT_TRUE(fixture.littleFsService.lastWriteData.find("Filetype: Flipper SubGhz RAW File") != std::string::npos);
    TEST_ASSERT_TRUE(fixture.view.contains("Saved file: /capture.sub"));
}

void test_unknown_command_displays_subghz_help() {
    SubGhzControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("wat"));

    TEST_ASSERT_TRUE(fixture.view.contains("Unknown command. Available SUBGHZ commands"));
}

}  // namespace subghz_controller_tests

void runSubGhzControllerTests() {
    using namespace subghz_controller_tests;
    RUN_TEST(test_config_updates_state_and_applies_scan_profile);
    RUN_TEST(test_config_reports_probe_failure_without_applying_profile);
    RUN_TEST(test_ensure_configured_prompts_once_after_success);
    RUN_TEST(test_setfreq_custom_updates_state_and_tunes_radio);
    RUN_TEST(test_send_builds_princeton_command_from_numeric_payload);
    RUN_TEST(test_send_rejects_missing_payload_without_touching_service);
    RUN_TEST(test_raw_prints_pulses_and_stops_sniffer);
    RUN_TEST(test_scan_detects_peak_and_saves_best_frequency);
    RUN_TEST(test_scan_reports_missing_radio_without_tuning);
    RUN_TEST(test_sweep_analyzes_one_frequency_and_stops);
    RUN_TEST(test_decode_analyzes_received_frame_and_stops_sniffer);
    RUN_TEST(test_receive_routes_to_decode_by_default);
    RUN_TEST(test_receive_routes_to_raw_when_decode_is_declined);
    RUN_TEST(test_replay_raw_captures_and_sends_once);
    RUN_TEST(test_replay_raw_can_be_cancelled_before_capture);
    RUN_TEST(test_jam_single_frequency_toggles_gdo_until_enter);
    RUN_TEST(test_jam_can_be_cancelled_before_transmit);
    RUN_TEST(test_bruteforce_sends_first_code_then_stops);
    RUN_TEST(test_trace_draws_logic_trace_then_stops);
    RUN_TEST(test_waterfall_draws_one_frequency_and_restores_previous_frequency);
    RUN_TEST(test_ear_maps_rssi_to_i2s_tone_until_enter);
    RUN_TEST(test_load_sends_selected_sub_file_command);
    RUN_TEST(test_load_reports_missing_sub_files);
    RUN_TEST(test_record_saves_raw_capture_as_sub_file);
    RUN_TEST(test_unknown_command_displays_subghz_help);
}
