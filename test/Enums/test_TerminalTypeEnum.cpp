#include <unity.h>

#include "Enums/TerminalTypeEnum.h"

namespace terminal_type_enum_tests {

void test_terminal_type_mapper_keeps_user_facing_labels_and_unknown_fallback() {
    TEST_ASSERT_EQUAL_STRING("USB Serial",
                             TerminalTypeEnumMapper::toString(TerminalTypeEnum::SerialPort).c_str());
    TEST_ASSERT_EQUAL_STRING("WiFi Hotspot",
                             TerminalTypeEnumMapper::toString(TerminalTypeEnum::WiFiAp).c_str());
    TEST_ASSERT_EQUAL_STRING("WiFi Connect",
                             TerminalTypeEnumMapper::toString(TerminalTypeEnum::WiFiClient).c_str());
    TEST_ASSERT_EQUAL_STRING("Standalone",
                             TerminalTypeEnumMapper::toString(TerminalTypeEnum::Standalone).c_str());
    TEST_ASSERT_EQUAL_STRING("Unknown",
                             TerminalTypeEnumMapper::toString(static_cast<TerminalTypeEnum>(99)).c_str());
}

}  // namespace terminal_type_enum_tests

void runTerminalTypeEnumTests() {
    using namespace terminal_type_enum_tests;
    RUN_TEST(test_terminal_type_mapper_keeps_user_facing_labels_and_unknown_fallback);
}
