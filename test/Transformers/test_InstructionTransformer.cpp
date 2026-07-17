#include <unity.h>

#include "Transformers/InstructionTransformer.h"

namespace instruction_transformer_tests {

InstructionTransformer transformer;

void test_instruction_detection_accepts_supported_prefixes_after_whitespace() {
    TEST_ASSERT_TRUE(transformer.isInstructionCommand("  [0xAA r]"));
    TEST_ASSERT_TRUE(transformer.isInstructionCommand(" >data"));
    TEST_ASSERT_TRUE(transformer.isInstructionCommand(" {macro"));
    TEST_ASSERT_FALSE(transformer.isInstructionCommand("send 0xAA"));
    TEST_ASSERT_FALSE(transformer.isInstructionCommand("   "));
}

void test_instruction_transform_extracts_raw_prefix_and_tokens() {
    const auto instructions = transformer.transform("[0xA5 r:8]");
    TEST_ASSERT_EQUAL_UINT32(1, instructions.size());
    TEST_ASSERT_EQUAL_CHAR('[', instructions[0].prefix);
    TEST_ASSERT_EQUAL_STRING("[0xA5 r:8]", instructions[0].raw.c_str());
    TEST_ASSERT_EQUAL_UINT32(2, instructions[0].tokens.size());
    TEST_ASSERT_EQUAL_STRING("0xA5", instructions[0].tokens[0].c_str());
    TEST_ASSERT_EQUAL_STRING("r:8", instructions[0].tokens[1].c_str());
}

void test_instruction_transform_supports_multiple_blocks() {
    const auto instructions = transformer.transform("[1 r] [2 D]");
    TEST_ASSERT_EQUAL_UINT32(2, instructions.size());
    TEST_ASSERT_EQUAL_STRING("1", instructions[0].tokens[0].c_str());
    TEST_ASSERT_EQUAL_STRING("2", instructions[1].tokens[0].c_str());
}

void test_instruction_transform_keeps_quoted_strings_as_single_tokens() {
    const auto instructions = transformer.transform("[\"AT OK\" 'Z']");
    TEST_ASSERT_EQUAL_UINT32(1, instructions.size());
    TEST_ASSERT_EQUAL_UINT32(2, instructions[0].tokens.size());
    TEST_ASSERT_EQUAL_STRING("\"AT OK\"", instructions[0].tokens[0].c_str());
    TEST_ASSERT_EQUAL_STRING("'Z'", instructions[0].tokens[1].c_str());
}

void test_instruction_transform_ignores_empty_and_unclosed_blocks() {
    TEST_ASSERT_TRUE(transformer.transform("").empty());
    TEST_ASSERT_TRUE(transformer.transform("[0xAA r").empty());
}

void test_instruction_bytecode_wraps_bracket_block_with_start_and_stop() {
    const auto instructions = transformer.transform("[0xA5 42 'Z']");
    const auto bytecodes = transformer.transformByteCode(instructions[0]);
    TEST_ASSERT_EQUAL_UINT32(5, bytecodes.size());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ByteCodeEnum::Start), static_cast<int>(bytecodes[0].getCommand()));
    TEST_ASSERT_EQUAL_HEX8(0xA5, bytecodes[1].getData());
    TEST_ASSERT_EQUAL_UINT8(42, bytecodes[2].getData());
    TEST_ASSERT_EQUAL_UINT8('Z', bytecodes[3].getData());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ByteCodeEnum::Stop), static_cast<int>(bytecodes[4].getCommand()));
}

void test_instruction_bytecode_expands_string_literal_to_writes() {
    const auto instructions = transformer.transform("[\"AT\"]");
    const auto bytecodes = transformer.transformByteCode(instructions[0]);
    TEST_ASSERT_EQUAL_UINT32(4, bytecodes.size());
    TEST_ASSERT_EQUAL_UINT8('A', bytecodes[1].getData());
    TEST_ASSERT_EQUAL_UINT8('T', bytecodes[2].getData());
}

void test_instruction_bytecode_supports_repeat_syntax_and_guide_comma_separator() {
    const auto instructions = transformer.transform("[d:10, r:300]");
    const auto bytecodes = transformer.transformByteCode(instructions[0]);
    TEST_ASSERT_EQUAL_UINT32(4, bytecodes.size());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ByteCodeEnum::DelayUs), static_cast<int>(bytecodes[1].getCommand()));
    TEST_ASSERT_EQUAL_UINT32(10, bytecodes[1].getRepeat());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ByteCodeEnum::Read), static_cast<int>(bytecodes[2].getCommand()));
    TEST_ASSERT_EQUAL_UINT32(255, bytecodes[2].getRepeat());
}

void test_instruction_bytecode_collapses_repeated_symbols() {
    const auto instructions = transformer.transform("[rrr hh]");
    const auto bytecodes = transformer.transformByteCode(instructions[0]);
    TEST_ASSERT_EQUAL_UINT32(4, bytecodes.size());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ByteCodeEnum::Read), static_cast<int>(bytecodes[1].getCommand()));
    TEST_ASSERT_EQUAL_UINT32(3, bytecodes[1].getRepeat());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ByteCodeEnum::AuxHigh), static_cast<int>(bytecodes[2].getCommand()));
    TEST_ASSERT_EQUAL_UINT32(2, bytecodes[2].getRepeat());
}

void test_instruction_bytecode_ignores_out_of_range_numeric_writes() {
    const auto instructions = transformer.transform("[256 0x100 255 0xFF]");
    const auto bytecodes = transformer.transformByteCode(instructions[0]);
    TEST_ASSERT_EQUAL_UINT32(4, bytecodes.size());
    TEST_ASSERT_EQUAL_UINT8(255, bytecodes[1].getData());
    TEST_ASSERT_EQUAL_UINT8(255, bytecodes[2].getData());
}

void test_instruction_transform_bytecodes_concatenates_blocks_in_order() {
    const auto instructions = transformer.transform("[1] [2]");
    const auto bytecodes = transformer.transformByteCodes(instructions);
    TEST_ASSERT_EQUAL_UINT32(6, bytecodes.size());
    TEST_ASSERT_EQUAL_UINT8(1, bytecodes[1].getData());
    TEST_ASSERT_EQUAL_UINT8(2, bytecodes[4].getData());
}

}  // namespace instruction_transformer_tests

void runInstructionTransformerTests() {
    using namespace instruction_transformer_tests;
    RUN_TEST(test_instruction_detection_accepts_supported_prefixes_after_whitespace);
    RUN_TEST(test_instruction_transform_extracts_raw_prefix_and_tokens);
    RUN_TEST(test_instruction_transform_supports_multiple_blocks);
    RUN_TEST(test_instruction_transform_keeps_quoted_strings_as_single_tokens);
    RUN_TEST(test_instruction_transform_ignores_empty_and_unclosed_blocks);
    RUN_TEST(test_instruction_bytecode_wraps_bracket_block_with_start_and_stop);
    RUN_TEST(test_instruction_bytecode_expands_string_literal_to_writes);
    RUN_TEST(test_instruction_bytecode_supports_repeat_syntax_and_guide_comma_separator);
    RUN_TEST(test_instruction_bytecode_collapses_repeated_symbols);
    RUN_TEST(test_instruction_bytecode_ignores_out_of_range_numeric_writes);
    RUN_TEST(test_instruction_transform_bytecodes_concatenates_blocks_in_order);
}
