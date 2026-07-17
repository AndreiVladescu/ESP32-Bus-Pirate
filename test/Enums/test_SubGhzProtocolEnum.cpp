#include <unity.h>

#include "Enums/SubGhzProtocolEnum.h"

namespace subghz_protocol_enum_tests {

void test_subghz_mapper_accepts_common_aliases_and_unknown_fallback() {
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SubGhzProtocolEnum::RAW),
                          static_cast<int>(SubGhzProtocolEnumMapper::fromString("raw")));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SubGhzProtocolEnum::BinRAW),
                          static_cast<int>(SubGhzProtocolEnumMapper::fromString("bin-raw")));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SubGhzProtocolEnum::RcSwitch),
                          static_cast<int>(SubGhzProtocolEnumMapper::fromString("rc_switch")));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SubGhzProtocolEnum::Princeton),
                          static_cast<int>(SubGhzProtocolEnumMapper::fromString("pt2262")));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SubGhzProtocolEnum::Unknown),
                          static_cast<int>(SubGhzProtocolEnumMapper::fromString("nope")));
}

void test_subghz_mapper_returns_canonical_names() {
    const auto names = SubGhzProtocolEnumMapper::getProtocolNames({
        SubGhzProtocolEnum::RAW,
        SubGhzProtocolEnum::BinRAW,
        SubGhzProtocolEnum::RcSwitch,
        SubGhzProtocolEnum::Princeton
    });

    TEST_ASSERT_EQUAL_UINT32(4, names.size());
    TEST_ASSERT_EQUAL_STRING("RAW", names[0].c_str());
    TEST_ASSERT_EQUAL_STRING("BinRAW", names[1].c_str());
    TEST_ASSERT_EQUAL_STRING("RcSwitch", names[2].c_str());
    TEST_ASSERT_EQUAL_STRING("Princeton", names[3].c_str());
}

}  // namespace subghz_protocol_enum_tests

void runSubGhzProtocolEnumTests() {
    using namespace subghz_protocol_enum_tests;
    RUN_TEST(test_subghz_mapper_accepts_common_aliases_and_unknown_fallback);
    RUN_TEST(test_subghz_mapper_returns_canonical_names);
}
