#include <algorithm>
#include <unity.h>

#include "Enums/LedProtocolEnum.h"

namespace led_protocol_enum_tests {

void test_led_protocol_mapper_accepts_case_insensitive_names_and_unknown_fallback() {
    TEST_ASSERT_EQUAL_INT(static_cast<int>(LedProtocolEnum::WS2812),
                          static_cast<int>(LedProtocolEnumMapper::fromString("WS2812")));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(LedProtocolEnum::LPD1886_8BIT),
                          static_cast<int>(LedProtocolEnumMapper::fromString("lpd1886_8bit")));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(LedProtocolEnum::UNKNOWN),
                          static_cast<int>(LedProtocolEnumMapper::fromString("definitely-not-a-protocol")));
    TEST_ASSERT_EQUAL_STRING("sk6812",
                             LedProtocolEnumMapper::toString(LedProtocolEnum::SK6812).c_str());
    TEST_ASSERT_EQUAL_STRING("unknown",
                             LedProtocolEnumMapper::toString(static_cast<LedProtocolEnum>(999)).c_str());
}

void test_led_protocol_mapper_lists_only_real_protocols() {
    const auto protocols = LedProtocolEnumMapper::getAllProtocols();

    TEST_ASSERT_FALSE(protocols.empty());
    for (const auto& protocol : protocols) {
        TEST_ASSERT_FALSE(protocol == "unknown");
    }
    TEST_ASSERT_TRUE(std::find(protocols.begin(), protocols.end(), "ws2812") != protocols.end());
    TEST_ASSERT_TRUE(std::find(protocols.begin(), protocols.end(), "sk6812") != protocols.end());
}

}  // namespace led_protocol_enum_tests

void runLedProtocolEnumTests() {
    using namespace led_protocol_enum_tests;
    RUN_TEST(test_led_protocol_mapper_accepts_case_insensitive_names_and_unknown_fallback);
    RUN_TEST(test_led_protocol_mapper_lists_only_real_protocols);
}
