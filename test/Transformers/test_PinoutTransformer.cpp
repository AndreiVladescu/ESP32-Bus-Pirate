#include <unity.h>

#include "Transformers/PinoutTransformer.h"

namespace pinout_transformer_tests {

PinoutTransformer transformer;

void test_pinout_transformer_builds_uart_mapping_from_global_state() {
    auto& state = GlobalState::getInstance();
    state.setUartTxPin(4);
    state.setUartRxPin(5);
    state.setUartBaudRate(115200);
    state.setUartDataBits(8);

    const auto config = transformer.build(ModeEnum::UART);

    TEST_ASSERT_EQUAL_STRING("UART", config.getMode().c_str());
    TEST_ASSERT_EQUAL_UINT32(4, config.getMappings().size());
    TEST_ASSERT_EQUAL_STRING("TX GPIO 4", config.getMappingAt(0).c_str());
    TEST_ASSERT_EQUAL_STRING("RX GPIO 5", config.getMappingAt(1).c_str());
    TEST_ASSERT_EQUAL_STRING("BAUD 115200", config.getMappingAt(2).c_str());
    TEST_ASSERT_EQUAL_STRING("BITS 8", config.getMappingAt(3).c_str());
}

void test_pinout_transformer_builds_i2c_mapping() {
    auto& state = GlobalState::getInstance();
    state.setI2cSdaPin(6);
    state.setI2cSclPin(7);
    state.setI2cFrequency(400000);

    const auto config = transformer.build(ModeEnum::I2C);

    TEST_ASSERT_EQUAL_STRING("SDA GPIO 6", config.getMappingAt(0).c_str());
    TEST_ASSERT_EQUAL_STRING("SCL GPIO 7", config.getMappingAt(1).c_str());
    TEST_ASSERT_EQUAL_STRING("FREQ 400000", config.getMappingAt(2).c_str());
}

void test_pinout_transformer_limits_jtag_mapping_to_four_lines() {
    GlobalState::getInstance().setJtagScanPins({1, 2, 3, 4, 5, 6});

    const auto config = transformer.build(ModeEnum::JTAG);

    TEST_ASSERT_EQUAL_UINT32(4, config.getMappings().size());
    TEST_ASSERT_EQUAL_STRING("SCAN GPIO 1", config.getMappingAt(0).c_str());
    TEST_ASSERT_EQUAL_STRING("SCAN GPIO 4 ...", config.getMappingAt(3).c_str());
}

void test_pinout_transformer_builds_can_spi_mapping() {
    auto& state = GlobalState::getInstance();
    state.setCanCspin(1);
    state.setCanSckPin(2);
    state.setCanSiPin(3);
    state.setCanSoPin(4);

    const auto config = transformer.build(ModeEnum::CAN_);

    TEST_ASSERT_EQUAL_STRING("CS GPIO 1", config.getMappingAt(0).c_str());
    TEST_ASSERT_EQUAL_STRING("SCK GPIO 2", config.getMappingAt(1).c_str());
    TEST_ASSERT_EQUAL_STRING("SI GPIO 3", config.getMappingAt(2).c_str());
    TEST_ASSERT_EQUAL_STRING("SO GPIO 4", config.getMappingAt(3).c_str());
}

void test_pinout_transformer_builds_led_protocol_mapping() {
    auto& state = GlobalState::getInstance();
    state.setLedDataPin(8);
    state.setLedClockPin(9);
    state.setLedLength(16);
    state.setLedProtocol("apa102");

    const auto config = transformer.build(ModeEnum::LED);

    TEST_ASSERT_EQUAL_STRING("DATA GPIO 8", config.getMappingAt(0).c_str());
    TEST_ASSERT_EQUAL_STRING("CLOCK GPIO 9", config.getMappingAt(1).c_str());
    TEST_ASSERT_EQUAL_STRING("LED COUNT 16", config.getMappingAt(2).c_str());
    TEST_ASSERT_EQUAL_STRING("apa102", config.getMappingAt(3).c_str());
}

}  // namespace pinout_transformer_tests

void runPinoutTransformerTests() {
    using namespace pinout_transformer_tests;
    RUN_TEST(test_pinout_transformer_builds_uart_mapping_from_global_state);
    RUN_TEST(test_pinout_transformer_builds_i2c_mapping);
    RUN_TEST(test_pinout_transformer_limits_jtag_mapping_to_four_lines);
    RUN_TEST(test_pinout_transformer_builds_can_spi_mapping);
    RUN_TEST(test_pinout_transformer_builds_led_protocol_mapping);
}
