#include <unity.h>

#include "Enums/InfraredProtocolEnum.h"

namespace infrared_protocol_enum_tests {

void test_infrared_mapper_covers_common_protocols_aliases_and_fallbacks() {
    TEST_ASSERT_EQUAL_INT(static_cast<int>(_NEC),
                          static_cast<int>(InfraredProtocolMapper::toEnum("NEC")));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(_RC5),
                          static_cast<int>(InfraredProtocolMapper::toEnum("rc5")));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(_RAW),
                          static_cast<int>(InfraredProtocolMapper::toEnum("raw")));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(_NEC2),
                          static_cast<int>(InfraredProtocolMapper::toEnum("does-not-exist")));
    TEST_ASSERT_EQUAL_STRING("raw", InfraredProtocolMapper::toString(_RAW).c_str());
    TEST_ASSERT_EQUAL_STRING("nec2",
                             InfraredProtocolMapper::toString(static_cast<InfraredProtocolEnum>(999)).c_str());
}

void test_infrared_mapper_keeps_known_round_trips_stable() {
    const InfraredProtocolEnum protocols[] = {_RC5, _RC6, _RAW, SAMSUNG32, _LG};

    for (const auto protocol : protocols) {
        const auto name = InfraredProtocolMapper::toString(protocol);
        TEST_ASSERT_EQUAL_INT(static_cast<int>(protocol),
                              static_cast<int>(InfraredProtocolMapper::toEnum(name)));
    }
}

}  // namespace infrared_protocol_enum_tests

void runInfraredProtocolEnumTests() {
    using namespace infrared_protocol_enum_tests;
    RUN_TEST(test_infrared_mapper_covers_common_protocols_aliases_and_fallbacks);
    RUN_TEST(test_infrared_mapper_keeps_known_round_trips_stable);
}
