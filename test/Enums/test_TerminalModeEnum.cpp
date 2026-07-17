#include <unity.h>

#include "Enums/TerminalModeEnum.h"

namespace terminal_mode_enum_tests {

void test_terminal_mode_mapper_keeps_labels_and_unknown_fallback() {
    TEST_ASSERT_EQUAL_STRING("Serial",
                             TerminalModeEnumMapper::toString(TerminalMode::SerialPort).c_str());
    TEST_ASSERT_EQUAL_STRING("Web   ",
                             TerminalModeEnumMapper::toString(TerminalMode::Web).c_str());
    TEST_ASSERT_EQUAL_STRING("None  ",
                             TerminalModeEnumMapper::toString(TerminalMode::None).c_str());
    TEST_ASSERT_EQUAL_STRING("Unknown",
                             TerminalModeEnumMapper::toString(static_cast<TerminalMode>(99)).c_str());
}

}  // namespace terminal_mode_enum_tests

void runTerminalModeEnumTests() {
    using namespace terminal_mode_enum_tests;
    RUN_TEST(test_terminal_mode_mapper_keeps_labels_and_unknown_fallback);
}
