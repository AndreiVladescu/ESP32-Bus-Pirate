#include <unity.h>

#include <string>

#include "Transformers/TerminalCommandTransformer.h"

namespace terminal_command_transformer_tests {

TerminalCommandTransformer transformer;

void test_terminal_command_transform_splits_root_subcommand_and_tail_args() {
    const auto cmd = transformer.transform("SEND 0x123 AA BB");
    TEST_ASSERT_EQUAL_STRING("send", cmd.getRoot().c_str());
    TEST_ASSERT_EQUAL_STRING("0x123", cmd.getSubcommand().c_str());
    TEST_ASSERT_EQUAL_STRING("AA BB", cmd.getArgs().c_str());
}

void test_terminal_command_transform_trims_outer_and_repeated_spaces() {
    const auto cmd = transformer.transform("  read    7   extra value  \r\n");
    TEST_ASSERT_EQUAL_STRING("read", cmd.getRoot().c_str());
    TEST_ASSERT_EQUAL_STRING("7", cmd.getSubcommand().c_str());
    TEST_ASSERT_EQUAL_STRING("extra value", cmd.getArgs().c_str());
}

void test_terminal_command_transform_preserves_uppercase_pull_aliases() {
    TEST_ASSERT_EQUAL_STRING("P", transformer.transform("P").getRoot().c_str());
    TEST_ASSERT_EQUAL_STRING("p", transformer.transform("p").getRoot().c_str());
}

void test_terminal_command_transform_turns_protocol_name_into_mode_command() {
    const auto cmd = transformer.transform("UART");
    TEST_ASSERT_EQUAL_STRING("mode", cmd.getRoot().c_str());
    TEST_ASSERT_EQUAL_STRING("uart", cmd.getSubcommand().c_str());
}

void test_terminal_command_transform_expands_mode_alias() {
    const auto cmd = transformer.transform("m nfc");
    TEST_ASSERT_EQUAL_STRING("mode", cmd.getRoot().c_str());
    TEST_ASSERT_EQUAL_STRING("rfid", cmd.getSubcommand().c_str());
}

void test_terminal_command_transform_corrects_unique_one_edit_typo() {
    const auto cmd = transformer.transform("hlep");
    TEST_ASSERT_EQUAL_STRING("help", cmd.getRoot().c_str());
}

void test_terminal_command_transform_many_splits_pipeline_and_skips_empty_segments() {
    const auto commands = transformer.transformMany(" status || || send 1 AA || read 2 ");
    TEST_ASSERT_EQUAL_UINT32(3, commands.size());
    TEST_ASSERT_EQUAL_STRING("status", commands[0].getRoot().c_str());
    TEST_ASSERT_EQUAL_STRING("send", commands[1].getRoot().c_str());
    TEST_ASSERT_EQUAL_STRING("read", commands[2].getRoot().c_str());
}

void test_terminal_command_transform_many_ignores_blank_input() {
    TEST_ASSERT_TRUE(transformer.transformMany(" \t\r\n ").empty());
}

void test_terminal_command_transform_many_limits_pipeline_to_64_commands() {
    std::string raw;
    for (int i = 0; i < 70; ++i) {
        if (!raw.empty()) raw += " || ";
        raw += "status";
    }
    TEST_ASSERT_EQUAL_UINT32(64, transformer.transformMany(raw).size());
}

void test_terminal_command_pipeline_detection_excludes_instruction_blocks() {
    TEST_ASSERT_TRUE(transformer.isPipelineCommand("read 1 || read 2"));
    TEST_ASSERT_FALSE(transformer.isPipelineCommand("  [0x01 || 0x02]"));
    TEST_ASSERT_FALSE(transformer.isPipelineCommand("{a || b}"));
}

void test_terminal_command_macro_detection_ignores_leading_whitespace() {
    TEST_ASSERT_TRUE(transformer.isMacroCommand("  (demo)"));
    TEST_ASSERT_FALSE(transformer.isMacroCommand("demo"));
    TEST_ASSERT_FALSE(transformer.isMacroCommand("   "));
}

void test_terminal_command_builtin_detection_requires_complete_command() {
    TEST_ASSERT_TRUE(transformer.isBuiltinCommand(" HELP "));
    TEST_ASSERT_FALSE(transformer.isBuiltinCommand("help extra"));
    TEST_ASSERT_FALSE(transformer.isBuiltinCommand("not-a-command"));
}

void test_terminal_command_repeat_detection_checks_word_boundary() {
    TEST_ASSERT_TRUE(transformer.isRepeatCommand("repeat"));
    TEST_ASSERT_TRUE(transformer.isRepeatCommand(" REPEAT 3 status "));
    TEST_ASSERT_FALSE(transformer.isRepeatCommand("repeater 3 status"));
}

void test_terminal_command_repeat_expands_single_command() {
    const auto commands = transformer.transformRepeatCommand("repeat 3 send 1 AA");
    TEST_ASSERT_EQUAL_UINT32(3, commands.size());
    for (const auto& cmd : commands) {
        TEST_ASSERT_EQUAL_STRING("send", cmd.getRoot().c_str());
        TEST_ASSERT_EQUAL_STRING("1", cmd.getSubcommand().c_str());
        TEST_ASSERT_EQUAL_STRING("AA", cmd.getArgs().c_str());
    }
}

void test_terminal_command_repeat_expands_pipeline_in_order() {
    const auto commands = transformer.transformRepeatCommand("repeat 2 read 1 || write 2");
    TEST_ASSERT_EQUAL_UINT32(4, commands.size());
    TEST_ASSERT_EQUAL_STRING("read", commands[0].getRoot().c_str());
    TEST_ASSERT_EQUAL_STRING("write", commands[1].getRoot().c_str());
    TEST_ASSERT_EQUAL_STRING("read", commands[2].getRoot().c_str());
    TEST_ASSERT_EQUAL_STRING("write", commands[3].getRoot().c_str());
}

void test_terminal_command_repeat_rejects_invalid_counts_and_nested_repeat() {
    TEST_ASSERT_TRUE(transformer.transformRepeatCommand("repeat 0 status").empty());
    TEST_ASSERT_TRUE(transformer.transformRepeatCommand("repeat 101 status").empty());
    TEST_ASSERT_TRUE(transformer.transformRepeatCommand("repeat nope status").empty());
    TEST_ASSERT_TRUE(transformer.transformRepeatCommand("repeat 2 repeat 3 status").empty());
    TEST_ASSERT_TRUE(transformer.transformRepeatCommand("repeat 2").empty());
}

void test_terminal_command_classifies_global_commands() {
    TEST_ASSERT_TRUE(transformer.isGlobalCommand(TerminalCommand("help")));
    TEST_ASSERT_TRUE(transformer.isGlobalCommand(TerminalCommand("delayus")));
    TEST_ASSERT_FALSE(transformer.isGlobalCommand(TerminalCommand("send")));
}

void test_terminal_command_classifies_screen_commands() {
    TEST_ASSERT_TRUE(transformer.isScreenCommand(TerminalCommand("config")));
    TEST_ASSERT_TRUE(transformer.isScreenCommand(TerminalCommand("receive")));
    TEST_ASSERT_FALSE(transformer.isScreenCommand(TerminalCommand("send")));
}

void test_terminal_command_tail_reassembles_subcommand_and_args() {
    TEST_ASSERT_EQUAL_STRING("one two three",
                             transformer.tail(TerminalCommand("root", "one", "two three")).c_str());
    TEST_ASSERT_EQUAL_STRING("args only",
                             transformer.tail(TerminalCommand("root", "", "args only")).c_str());
    TEST_ASSERT_EQUAL_STRING("", transformer.tail(TerminalCommand("root")).c_str());
}

}  // namespace terminal_command_transformer_tests

void runTerminalCommandTransformerTests() {
    using namespace terminal_command_transformer_tests;
    RUN_TEST(test_terminal_command_transform_splits_root_subcommand_and_tail_args);
    RUN_TEST(test_terminal_command_transform_trims_outer_and_repeated_spaces);
    RUN_TEST(test_terminal_command_transform_preserves_uppercase_pull_aliases);
    RUN_TEST(test_terminal_command_transform_turns_protocol_name_into_mode_command);
    RUN_TEST(test_terminal_command_transform_expands_mode_alias);
    RUN_TEST(test_terminal_command_transform_corrects_unique_one_edit_typo);
    RUN_TEST(test_terminal_command_transform_many_splits_pipeline_and_skips_empty_segments);
    RUN_TEST(test_terminal_command_transform_many_ignores_blank_input);
    RUN_TEST(test_terminal_command_transform_many_limits_pipeline_to_64_commands);
    RUN_TEST(test_terminal_command_pipeline_detection_excludes_instruction_blocks);
    RUN_TEST(test_terminal_command_macro_detection_ignores_leading_whitespace);
    RUN_TEST(test_terminal_command_builtin_detection_requires_complete_command);
    RUN_TEST(test_terminal_command_repeat_detection_checks_word_boundary);
    RUN_TEST(test_terminal_command_repeat_expands_single_command);
    RUN_TEST(test_terminal_command_repeat_expands_pipeline_in_order);
    RUN_TEST(test_terminal_command_repeat_rejects_invalid_counts_and_nested_repeat);
    RUN_TEST(test_terminal_command_classifies_global_commands);
    RUN_TEST(test_terminal_command_classifies_screen_commands);
    RUN_TEST(test_terminal_command_tail_reassembles_subcommand_and_args);
}
