#include <string>
#include <unity.h>

#include "Managers/AliasManager.h"

namespace alias_manager_tests {

void test_add_rejects_empty_key_or_value() {
    AliasManager manager;

    TEST_ASSERT_FALSE(manager.add("", "mode can"));
    TEST_ASSERT_FALSE(manager.add("can", ""));
    TEST_ASSERT_EQUAL_UINT32(0, manager.size());
}

void test_add_expand_remove_and_clear_aliases() {
    AliasManager manager;

    TEST_ASSERT_TRUE(manager.add("can", "mode can"));
    TEST_ASSERT_TRUE(manager.has("can"));
    TEST_ASSERT_EQUAL_UINT32(1, manager.size());
    TEST_ASSERT_EQUAL_STRING("mode can", manager.expand("can").c_str());

    const std::string passthrough = "mode i2c";
    TEST_ASSERT_EQUAL_STRING("mode i2c", manager.expand(passthrough).c_str());

    TEST_ASSERT_TRUE(manager.remove("can"));
    TEST_ASSERT_FALSE(manager.has("can"));
    TEST_ASSERT_FALSE(manager.remove("can"));

    TEST_ASSERT_TRUE(manager.add("i2c", "mode i2c"));
    manager.clear();
    TEST_ASSERT_EQUAL_UINT32(0, manager.size());
    TEST_ASSERT_FALSE(manager.has("i2c"));
}

void test_add_updates_existing_alias_without_growing_size() {
    AliasManager manager;

    TEST_ASSERT_TRUE(manager.add("scan", "i2c scan"));
    TEST_ASSERT_TRUE(manager.add("scan", "i2c scan 0x50"));

    TEST_ASSERT_EQUAL_UINT32(1, manager.size());
    TEST_ASSERT_EQUAL_STRING("i2c scan 0x50", manager.expand("scan").c_str());
}

void test_add_rejects_aliases_above_capacity() {
    AliasManager manager;

    for (size_t i = 0; i < manager.sizeMax(); ++i) {
        TEST_ASSERT_TRUE(manager.add("a" + std::to_string(i), "command" + std::to_string(i)));
    }

    TEST_ASSERT_EQUAL_UINT32(manager.sizeMax(), manager.size());
    TEST_ASSERT_FALSE(manager.add("overflow", "command"));
    TEST_ASSERT_EQUAL_UINT32(manager.sizeMax(), manager.size());
}

}  // namespace alias_manager_tests

void runAliasManagerTests() {
    using namespace alias_manager_tests;
    RUN_TEST(test_add_rejects_empty_key_or_value);
    RUN_TEST(test_add_expand_remove_and_clear_aliases);
    RUN_TEST(test_add_updates_existing_alias_without_growing_size);
    RUN_TEST(test_add_rejects_aliases_above_capacity);
}
