#include <unity.h>

#include <cstddef>
#include <string>
#include <vector>

#include "Analyzers/SubGhzAnalyzer.h"

namespace subghz_analyzer_tests {

bool containsText(const std::string& value, const std::string& text) {
    return value.find(text) != std::string::npos;
}

rmt_symbol_word_t symbol(uint32_t duration0, uint32_t level0, uint32_t duration1, uint32_t level1) {
    rmt_symbol_word_t s{};
    s.duration0 = duration0;
    s.level0 = level0;
    s.duration1 = duration1;
    s.level1 = level1;
    return s;
}

std::vector<rmt_symbol_word_t> pulseLengthFrame(const std::string& bits) {
    std::vector<rmt_symbol_word_t> out;
    out.reserve(bits.size());
    for (char bit : bits) {
        if (bit == '1') out.push_back(symbol(600, 1, 200, 0));
        else out.push_back(symbol(200, 1, 600, 0));
    }
    return out;
}

void test_analyze_frame_reports_empty_frame() {
    SubGhzAnalyzer analyzer;

    const std::string report = analyzer.analyzeFrame({});

    TEST_ASSERT_TRUE(containsText(report, "Pulse count  : 0"));
    TEST_ASSERT_TRUE(containsText(report, "Encoding     : Unknown"));
    TEST_ASSERT_TRUE(containsText(report, "Empty frame"));
}

void test_analyze_frame_detects_manchester_like_symbols() {
    SubGhzAnalyzer analyzer;
    std::vector<rmt_symbol_word_t> frame;
    for (int i = 0; i < 12; ++i) {
        frame.push_back(symbol(100, 1, 100, 0));
    }

    const std::string report = analyzer.analyzeFrame(frame);

    TEST_ASSERT_TRUE(containsText(report, "Encoding     : Manchester"));
    TEST_ASSERT_TRUE(containsText(report, "Protocol     : Manchester"));
    TEST_ASSERT_TRUE(containsText(report, "Base T (us)  : 100"));
}

void test_analyze_frame_decodes_binary_pulse_length_payload() {
    SubGhzAnalyzer analyzer;
    const auto frame = pulseLengthFrame("101010101010");

    const std::string report = analyzer.analyzeFrame(frame);

    TEST_ASSERT_TRUE(containsText(report, "Encoding     : PulseLength"));
    TEST_ASSERT_TRUE(containsText(report, "Bit count    : 12"));
    TEST_ASSERT_TRUE(containsText(report, "Payload      : AAA"));
    TEST_ASSERT_TRUE(containsText(report, "Protocol     : PT2262/EV1527-like"));
}

void test_analyze_frame_preserves_tristate_payloads() {
    SubGhzAnalyzer analyzer;
    const std::vector<rmt_symbol_word_t> frame = {
        symbol(200, 1, 200, 0),
        symbol(200, 1, 600, 0),
        symbol(200, 1, 200, 0),
        symbol(600, 1, 200, 0),
        symbol(200, 1, 600, 0),
        symbol(600, 1, 200, 0),
        symbol(200, 1, 200, 0),
        symbol(200, 1, 600, 0),
        symbol(200, 1, 200, 0),
        symbol(600, 1, 200, 0),
        symbol(200, 1, 600, 0),
        symbol(600, 1, 200, 0)
    };

    const std::string report = analyzer.analyzeFrame(frame);

    TEST_ASSERT_TRUE(containsText(report, "Encoding     : PulseLength"));
    TEST_ASSERT_TRUE(containsText(report, "Payload      : F0F101F0F101"));
    TEST_ASSERT_TRUE(containsText(report, "Protocol     : PT2262/SC2262-like"));
}

void test_analyze_frequency_activity_reports_no_hits() {
    SubGhzAnalyzer analyzer;

    const std::string report = analyzer.analyzeFrequencyActivity(
        50,
        10,
        -80,
        [](int) { return -100; }
    );

    TEST_ASSERT_TRUE(containsText(report, "peak=-100 dBm"));
    TEST_ASSERT_TRUE(containsText(report, "activity=0%"));
    TEST_ASSERT_TRUE(containsText(report, "conf=0%"));
    TEST_ASSERT_TRUE(containsText(report, "hits=0/5"));
}

void test_analyze_frequency_activity_guesses_ask_ook_from_bimodal_samples() {
    SubGhzAnalyzer analyzer;
    const std::vector<int> samples = {-95, -55, -94, -56, -95, -55, -94, -56};
    size_t index = 0;

    const std::string report = analyzer.analyzeFrequencyActivity(
        80,
        10,
        -80,
        [&](int) {
            return samples[index++ % samples.size()];
        }
    );

    TEST_ASSERT_TRUE(containsText(report, "peak=-55 dBm"));
    TEST_ASSERT_TRUE(containsText(report, "activity=50%"));
    TEST_ASSERT_TRUE(containsText(report, "hits=4/8"));
    TEST_ASSERT_TRUE(containsText(report, "mod=ASK/OOK"));
}

void test_analyze_frequency_activity_stops_when_abort_callback_triggers() {
    SubGhzAnalyzer analyzer;
    int calls = 0;

    const std::string report = analyzer.analyzeFrequencyActivity(
        50,
        10,
        -80,
        [&](int) {
            ++calls;
            return -60;
        },
        [&]() {
            return calls >= 2;
        }
    );

    TEST_ASSERT_EQUAL_INT(2, calls);
    TEST_ASSERT_TRUE(containsText(report, "hits=2/2"));
}

}  // namespace subghz_analyzer_tests

void runSubGhzAnalyzerTests() {
    using namespace subghz_analyzer_tests;
    RUN_TEST(test_analyze_frame_reports_empty_frame);
    RUN_TEST(test_analyze_frame_detects_manchester_like_symbols);
    RUN_TEST(test_analyze_frame_decodes_binary_pulse_length_payload);
    RUN_TEST(test_analyze_frame_preserves_tristate_payloads);
    RUN_TEST(test_analyze_frequency_activity_reports_no_hits);
    RUN_TEST(test_analyze_frequency_activity_guesses_ask_ook_from_bimodal_samples);
    RUN_TEST(test_analyze_frequency_activity_stops_when_abort_callback_triggers);
}

