#include <unity.h>

#include <cstdint>
#include <string>

#include "Analyzers/PinAnalyzer.h"
#include "../Services/FakePinService.h"
#include "../Services/FakeUtilityService.h"

namespace pin_analyzer_tests {

struct PinAnalyzerFixture {
    FakePinService pinService;
    FakeUtilityService utility;
    PinAnalyzer analyzer{pinService, utility};

    void begin(uint8_t pin, bool initialLevel, uint32_t startMs = 0) {
        utility.currentNowMs = startMs;
        pinService.readValues.push_back(initialLevel);
        analyzer.begin(pin);
    }

    void edge(uint8_t pin, bool level, uint32_t nowMs) {
        utility.currentNowMs = nowMs;
        pinService.readValues.push_back(level);
        analyzer.sample();
    }

    void end() {
        analyzer.end();
    }
};

void generateClock(PinAnalyzerFixture& fixture, uint8_t pin, uint32_t edges, uint32_t halfPeriodMs) {
    bool level = false;
    uint32_t nowMs = 0;

    for (uint32_t i = 0; i < edges; ++i) {
        nowMs += halfPeriodMs;
        level = !level;
        fixture.edge(pin, level, nowMs);
    }
}

void generatePwm(PinAnalyzerFixture& fixture, uint8_t pin, uint32_t edges, uint32_t lowMs, uint32_t highMs) {
    bool level = false;
    uint32_t nowMs = 0;

    for (uint32_t i = 0; i < edges; ++i) {
        nowMs += level ? highMs : lowMs;
        level = !level;
        fixture.edge(pin, level, nowMs);
    }
}

void test_begin_configures_pin_and_idle_report_stays_low() {
    PinAnalyzerFixture fixture;
    fixture.begin(7, false);
    fixture.utility.currentNowMs = 8000;

    auto report = fixture.analyzer.buildReport(false);
    const std::string formatted = fixture.analyzer.formatWizardReport(7, report);

    TEST_ASSERT_EQUAL_UINT32(1, fixture.pinService.inputCalls.size());
    TEST_ASSERT_EQUAL_UINT8(7, fixture.pinService.inputCalls[0]);
    TEST_ASSERT_EQUAL_UINT32(0, report.edges);
    TEST_ASSERT_EQUAL(PinAnalyzer::SignalKind::Idle, report.top1.kind);
    TEST_ASSERT_TRUE(formatted.find("I did not see any activity.") != std::string::npos);
    TEST_ASSERT_TRUE(formatted.find("stayed mostly LOW") != std::string::npos);

    fixture.end();
}

void test_should_report_after_analysis_window_and_reset_window_restarts_timer() {
    PinAnalyzerFixture fixture;
    fixture.begin(3, false, 100);

    TEST_ASSERT_FALSE(fixture.analyzer.shouldReport(8099));
    TEST_ASSERT_TRUE(fixture.analyzer.shouldReport(8100));

    fixture.utility.currentNowMs = 500;
    fixture.analyzer.resetWindow();

    TEST_ASSERT_FALSE(fixture.analyzer.shouldReport(8499));
    TEST_ASSERT_TRUE(fixture.analyzer.shouldReport(8500));

    fixture.end();
}

void test_regular_50_percent_signal_is_detected_as_clock() {
    PinAnalyzerFixture fixture;
    fixture.begin(4, false);

    generateClock(fixture, 4, 80, 1);
    fixture.utility.currentNowMs = 80;

    auto report = fixture.analyzer.buildReport(false);
    const std::string formatted = fixture.analyzer.formatWizardReport(4, report);

    TEST_ASSERT_EQUAL_UINT32(80, report.edges);
    TEST_ASSERT_EQUAL(PinAnalyzer::SignalKind::Clock, report.top1.kind);
    TEST_ASSERT_TRUE(report.top1.confidencePct >= 70);
    TEST_ASSERT_TRUE(report.dutyPct > 45.0f);
    TEST_ASSERT_TRUE(report.dutyPct < 55.0f);
    TEST_ASSERT_TRUE(formatted.find("Top guess: Clock") != std::string::npos);

    fixture.end();
}

void test_asymmetric_regular_signal_is_detected_as_pwm() {
    PinAnalyzerFixture fixture;
    fixture.begin(5, false);

    generatePwm(fixture, 5, 80, 3, 1);
    fixture.utility.currentNowMs = 160;

    auto report = fixture.analyzer.buildReport(false);
    const std::string formatted = fixture.analyzer.formatWizardReport(5, report);

    TEST_ASSERT_EQUAL_UINT32(80, report.edges);
    TEST_ASSERT_EQUAL(PinAnalyzer::SignalKind::PWM, report.top1.kind);
    TEST_ASSERT_TRUE(report.top1.confidencePct >= 60);
    TEST_ASSERT_TRUE(report.dutyPct > 20.0f);
    TEST_ASSERT_TRUE(report.dutyPct < 30.0f);
    TEST_ASSERT_TRUE(formatted.find("Top guess: PWM") != std::string::npos);

    fixture.end();
}

void test_reset_window_clears_previous_activity() {
    PinAnalyzerFixture fixture;
    fixture.begin(6, false);

    generateClock(fixture, 6, 20, 1);
    fixture.utility.currentNowMs = 20;
    fixture.analyzer.resetWindow();

    fixture.utility.currentNowMs = 8020;
    auto report = fixture.analyzer.buildReport(false);

    TEST_ASSERT_EQUAL_UINT32(0, report.edges);
    TEST_ASSERT_EQUAL(PinAnalyzer::SignalKind::Idle, report.top1.kind);

    fixture.end();
}

}  // namespace pin_analyzer_tests

void runPinAnalyzerTests() {
    using namespace pin_analyzer_tests;
    RUN_TEST(test_begin_configures_pin_and_idle_report_stays_low);
    RUN_TEST(test_should_report_after_analysis_window_and_reset_window_restarts_timer);
    RUN_TEST(test_regular_50_percent_signal_is_detected_as_clock);
    RUN_TEST(test_asymmetric_regular_signal_is_detected_as_pwm);
    RUN_TEST(test_reset_window_clears_previous_activity);
}
