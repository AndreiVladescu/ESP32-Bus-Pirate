#include <unity.h>

#include "Enums/ModeEnum.h"

namespace mode_enum_tests {

void test_mode_mapper_is_case_insensitive_and_preserves_wire_names() {
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ModeEnum::I2C),
                          static_cast<int>(ModeEnumMapper::fromString("i2c")));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ModeEnum::CAN_),
                          static_cast<int>(ModeEnumMapper::fromString("can")));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ModeEnum::OneWire),
                          static_cast<int>(ModeEnumMapper::fromString("1wire")));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ModeEnum::None),
                          static_cast<int>(ModeEnumMapper::fromString("nope")));
    TEST_ASSERT_EQUAL_STRING("CAN", ModeEnumMapper::toString(ModeEnum::CAN_).c_str());
    TEST_ASSERT_EQUAL_STRING("Unknown Protocol",
                             ModeEnumMapper::toString(static_cast<ModeEnum>(999)).c_str());
}

void test_mode_mapper_protocol_lists_exclude_none_and_return_names() {
    const auto protocols = ModeEnumMapper::getProtocols();
    const auto names = ModeEnumMapper::getProtocolNames({ModeEnum::HIZ, ModeEnum::I2C, ModeEnum::CAN_});

    TEST_ASSERT_FALSE(protocols.empty());
    for (const auto mode : protocols) {
        TEST_ASSERT_NOT_EQUAL(static_cast<int>(ModeEnum::None), static_cast<int>(mode));
    }
    TEST_ASSERT_EQUAL_UINT32(3, names.size());
    TEST_ASSERT_EQUAL_STRING("HIZ", names[0].c_str());
    TEST_ASSERT_EQUAL_STRING("I2C", names[1].c_str());
    TEST_ASSERT_EQUAL_STRING("CAN", names[2].c_str());
}

}  // namespace mode_enum_tests

void runModeEnumTests() {
    using namespace mode_enum_tests;
    RUN_TEST(test_mode_mapper_is_case_insensitive_and_preserves_wire_names);
    RUN_TEST(test_mode_mapper_protocol_lists_exclude_none_and_return_names);
}
