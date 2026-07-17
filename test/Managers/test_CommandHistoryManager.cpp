#include <string>
#include <unity.h>

#include "Managers/CommandHistoryManager.h"

namespace command_history_manager_tests {

void test_add_ignores_empty_lines_and_consecutive_duplicates() {
    CommandHistoryManager manager;

    manager.add("");
    manager.add("scan");
    manager.add("scan");
    manager.add("status");

    TEST_ASSERT_EQUAL_UINT32(2, manager.size());
    TEST_ASSERT_EQUAL_STRING("status", manager.up().c_str());
    TEST_ASSERT_EQUAL_STRING("scan", manager.up().c_str());
}

void test_up_down_and_reset_navigate_history() {
    CommandHistoryManager manager;
    manager.add("first");
    manager.add("second");
    manager.add("third");

    TEST_ASSERT_EQUAL_STRING("third", manager.up().c_str());
    TEST_ASSERT_EQUAL_STRING("second", manager.up().c_str());
    TEST_ASSERT_EQUAL_STRING("first", manager.up().c_str());
    TEST_ASSERT_EQUAL_STRING("first", manager.up().c_str());

    TEST_ASSERT_EQUAL_STRING("second", manager.down().c_str());
    TEST_ASSERT_EQUAL_STRING("third", manager.down().c_str());
    TEST_ASSERT_EQUAL_STRING("", manager.down().c_str());

    manager.reset();
    TEST_ASSERT_EQUAL_STRING("third", manager.up().c_str());
}

void test_autocomplete_returns_newest_matching_history_entry() {
    CommandHistoryManager manager;
    manager.add("i2c scan");
    manager.add("can status");
    manager.add("i2c read");

    TEST_ASSERT_EQUAL_STRING("i2c read", manager.autocomplete("i2c").c_str());
    TEST_ASSERT_EQUAL_STRING("", manager.down().c_str());
    TEST_ASSERT_EQUAL_STRING("can status", manager.autocomplete("can").c_str());
    TEST_ASSERT_EQUAL_STRING("", manager.autocomplete("spi").c_str());
    TEST_ASSERT_EQUAL_STRING("", manager.autocomplete("").c_str());
}

void test_history_keeps_last_fifty_entries() {
    CommandHistoryManager manager;

    for (int i = 0; i < 55; ++i) {
        manager.add("cmd" + std::to_string(i));
    }

    TEST_ASSERT_EQUAL_UINT32(50, manager.size());
    std::string oldestVisible;
    for (int i = 0; i < 50; ++i) {
        oldestVisible = manager.up();
    }

    TEST_ASSERT_EQUAL_STRING("cmd5", oldestVisible.c_str());
    TEST_ASSERT_EQUAL_STRING("cmd5", manager.up().c_str());
}

void test_empty_history_returns_empty_navigation_results() {
    CommandHistoryManager manager;

    TEST_ASSERT_EQUAL_STRING("", manager.up().c_str());
    TEST_ASSERT_EQUAL_STRING("", manager.down().c_str());
    TEST_ASSERT_EQUAL_STRING("", manager.autocomplete("anything").c_str());
}

}  // namespace command_history_manager_tests

void runCommandHistoryManagerTests() {
    using namespace command_history_manager_tests;
    RUN_TEST(test_add_ignores_empty_lines_and_consecutive_duplicates);
    RUN_TEST(test_up_down_and_reset_navigate_history);
    RUN_TEST(test_autocomplete_returns_newest_matching_history_entry);
    RUN_TEST(test_history_keeps_last_fifty_entries);
    RUN_TEST(test_empty_history_returns_empty_navigation_results);
}
