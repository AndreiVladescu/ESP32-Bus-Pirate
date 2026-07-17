#include <algorithm>
#include <unity.h>

#include "Enums/LedChipsetEnum.h"

namespace led_chipset_enum_tests {

void test_led_chipset_mapper_normalizes_case_and_falls_back_to_apa102() {
    TEST_ASSERT_EQUAL_INT(static_cast<int>(APA102),
                          static_cast<int>(LedChipsetMapper::fromString("APA102")));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SK9822HD),
                          static_cast<int>(LedChipsetMapper::fromString("sk9822hd")));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(APA102),
                          static_cast<int>(LedChipsetMapper::fromString("missing-chipset")));
    TEST_ASSERT_EQUAL_STRING("dotstar",
                             LedChipsetMapper::toString(DOTSTAR).c_str());
    TEST_ASSERT_EQUAL_STRING("apa102",
                             LedChipsetMapper::normalize("DOtStarMissing").c_str());
}

void test_led_chipset_mapper_lists_clocked_chipsets() {
    const auto chipsets = LedChipsetMapper::getAllChipsets();

    TEST_ASSERT_FALSE(chipsets.empty());
    TEST_ASSERT_TRUE(std::find(chipsets.begin(), chipsets.end(), "apa102") != chipsets.end());
    TEST_ASSERT_TRUE(std::find(chipsets.begin(), chipsets.end(), "hd107hd") != chipsets.end());
    TEST_ASSERT_TRUE(LedChipsetMapper::isClockBased("apa102"));
    TEST_ASSERT_TRUE(LedChipsetMapper::isClockBased("unknown-chipset"));
}

}  // namespace led_chipset_enum_tests

void runLedChipsetEnumTests() {
    using namespace led_chipset_enum_tests;
    RUN_TEST(test_led_chipset_mapper_normalizes_case_and_falls_back_to_apa102);
    RUN_TEST(test_led_chipset_mapper_lists_clocked_chipsets);
}
