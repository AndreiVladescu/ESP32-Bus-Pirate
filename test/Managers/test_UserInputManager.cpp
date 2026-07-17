#include <vector>
#include <unity.h>

#include <string>

#include "../Inputs/FakeInput.h"
#include "../Views/FakeTerminalView.h"
#include "Managers/UserInputManager.h"
#include "Transformers/ArgTransformer.h"

namespace user_input_manager_tests {

struct UserInputFixture {
    FakeTerminalView view;
    FakeInput input;
    ArgTransformer transformer;
    UserInputManager manager{view, input, transformer};
};

void queueEscLeft(FakeInput& input) {
    input.blockingChars.push_back('\x1B');
    input.blockingChars.push_back('[');
    input.blockingChars.push_back('D');
}

void test_get_line_supports_cursor_insertion() {
    UserInputFixture fixture;
    fixture.input.blockingChars.push_back('a');
    fixture.input.blockingChars.push_back('c');
    queueEscLeft(fixture.input);
    fixture.input.blockingChars.push_back('b');
    fixture.input.blockingChars.push_back('\n');

    TEST_ASSERT_EQUAL_STRING("abc", fixture.manager.getLine().c_str());
}

void test_get_line_only_number_filters_non_digits() {
    UserInputFixture fixture;
    fixture.input.queueLine("a1b2");

    TEST_ASSERT_EQUAL_STRING("12", fixture.manager.getLine(true).c_str());
}

void test_get_line_supports_backspace_at_cursor_and_length_limit() {
    UserInputFixture fixture;
    fixture.input.blockingChars.push_back('a');
    fixture.input.blockingChars.push_back('b');
    fixture.input.blockingChars.push_back('c');
    fixture.input.blockingChars.push_back('d');
    queueEscLeft(fixture.input);
    queueEscLeft(fixture.input);
    fixture.input.blockingChars.push_back('\b');
    fixture.input.blockingChars.push_back('\n');

    TEST_ASSERT_EQUAL_STRING("acd", fixture.manager.getLine().c_str());

    UserInputFixture longFixture;
    longFixture.input.queueLine(std::string(300, 'x'));

    TEST_ASSERT_EQUAL_UINT32(256, longFixture.manager.getLine().size());
}

void test_read_string_returns_default_on_empty_or_user_value() {
    UserInputFixture fixture;
    fixture.input.queueLine("");
    fixture.input.queueLine("custom");

    TEST_ASSERT_EQUAL_STRING("default", fixture.manager.readString("Name", "default").c_str());
    TEST_ASSERT_EQUAL_STRING("custom", fixture.manager.readString("Name", "default").c_str());
}

void test_read_sanitized_string_filters_or_retries_until_content_remains() {
    UserInputFixture fixture;
    fixture.input.queueLine("");
    fixture.input.queueLine("A b!_+9-");
    fixture.input.queueLine("$$$");
    fixture.input.queueLine("A1_b-+");

    TEST_ASSERT_EQUAL_STRING("def", fixture.manager.readSanitizedString("Name", "def").c_str());
    TEST_ASSERT_EQUAL_STRING("Ab_+9-", fixture.manager.readSanitizedString("Name", "def").c_str());
    TEST_ASSERT_EQUAL_STRING("Ab", fixture.manager.readSanitizedString("Name", "def", true).c_str());
    TEST_ASSERT_TRUE(fixture.view.contains("Invalid input. Allowed: letters"));
}

void test_read_validated_phone_number_retries_format_length_and_accepts_empty() {
    UserInputFixture fixture;
    fixture.input.queueLine("");
    fixture.input.queueLine("123456");
    fixture.input.queueLine("+12A456");
    fixture.input.queueLine("+12");
    fixture.input.queueLine("+123456");

    TEST_ASSERT_EQUAL_STRING("", fixture.manager.readValidatedPhoneNumber("Phone").c_str());
    TEST_ASSERT_EQUAL_STRING("+123456", fixture.manager.readValidatedPhoneNumber("Phone", 4, 6).c_str());
    TEST_ASSERT_TRUE(fixture.view.contains("Phone number must start with '+'"));
    TEST_ASSERT_TRUE(fixture.view.contains("Only digits allowed after '+'"));
    TEST_ASSERT_TRUE(fixture.view.contains("Invalid length."));
}

void test_read_yes_no_uses_default_and_retries_invalid_answer() {
    UserInputFixture fixture;
    fixture.input.queueLine("");
    fixture.input.queueLine("maybe");
    fixture.input.queueLine("n");

    TEST_ASSERT_TRUE(fixture.manager.readYesNo("Enable", true));
    TEST_ASSERT_FALSE(fixture.manager.readYesNo("Enable", true));
    TEST_ASSERT_TRUE(fixture.view.contains("Please answer y or n."));
}

void test_read_validated_uint8_accepts_hex_and_retries_out_of_range() {
    UserInputFixture fixture;
    fixture.input.queueLine("0x20");
    fixture.input.queueLine("300");
    fixture.input.queueLine("42");

    TEST_ASSERT_EQUAL_UINT8(32, fixture.manager.readValidatedUint8("Value", 0, 0, 100));
    TEST_ASSERT_EQUAL_UINT8(42, fixture.manager.readValidatedUint8("Value", 0, 0, 100));
    TEST_ASSERT_TRUE(fixture.view.contains("Must be 0-100."));
}

void test_read_validated_uint16_and_uint32_accept_hex_defaults_and_retries_invalid() {
    UserInputFixture fixture;
    fixture.input.queueLine("");
    fixture.input.queueLine("0x10000");
    fixture.input.queueLine("0xBEEF");
    fixture.input.queueLine("");
    fixture.input.queueLine("bad");
    fixture.input.queueLine("0xDEADBEEF");

    TEST_ASSERT_EQUAL_HEX16(0x1234, fixture.manager.readValidatedUint16("Word", 0x1234, true));
    TEST_ASSERT_EQUAL_HEX16(0xBEEF, fixture.manager.readValidatedUint16("Word", 0, true));
    TEST_ASSERT_EQUAL_HEX32(0x12345678, fixture.manager.readValidatedUint32("Dword", 0x12345678, true));
    TEST_ASSERT_EQUAL_HEX32(0xDEADBEEF, fixture.manager.readValidatedUint32("Dword", 0, true));
    TEST_ASSERT_TRUE(fixture.view.contains("Invalid number. Use decimal or 0x-prefixed hex."));
}

void test_read_validated_int_hex_and_byte_retry_invalid_and_range_errors() {
    UserInputFixture fixture;
    fixture.input.queueLine("");
    fixture.input.queueLine("nope");
    fixture.input.queueLine("0x100");
    fixture.input.queueLine("0x2A");
    fixture.input.queueLine("");
    fixture.input.queueLine("zz");
    fixture.input.queueLine("0x100");
    fixture.input.queueLine("0x2A");
    fixture.input.queueLine("");
    fixture.input.queueLine("999");
    fixture.input.queueLine("-5");

    TEST_ASSERT_EQUAL_HEX32(0x10, fixture.manager.readValidatedHex("Hex", 0x10, 0, 0xFF));
    TEST_ASSERT_EQUAL_HEX32(0x2A, fixture.manager.readValidatedHex("Hex", 0, 0, 0xFF));
    TEST_ASSERT_EQUAL_UINT8(7, fixture.manager.readValidatedByte("Byte", 7, true));
    TEST_ASSERT_EQUAL_UINT8(0x2A, fixture.manager.readValidatedByte("Byte", 0, true));
    TEST_ASSERT_EQUAL_INT(3, fixture.manager.readValidatedInt("Signed", 3, -10, 10));
    TEST_ASSERT_EQUAL_INT(-5, fixture.manager.readValidatedInt("Signed", 0, -10, 10));
    TEST_ASSERT_TRUE(fixture.view.contains("Out of range"));
    TEST_ASSERT_TRUE(fixture.view.contains("Value out of range"));
    TEST_ASSERT_TRUE(fixture.view.contains("Invalid input. Must be -10-10"));
}

void test_read_char_choice_uses_default_uppercases_and_retries_invalid() {
    UserInputFixture fixture;
    fixture.input.queueLine("");
    fixture.input.queueLine("x");
    fixture.input.queueLine("b");

    TEST_ASSERT_EQUAL_CHAR('A', fixture.manager.readCharChoice("Choice", 'A', {'A', 'B'}));
    TEST_ASSERT_EQUAL_CHAR('B', fixture.manager.readCharChoice("Choice", 'A', {'A', 'B'}));
    TEST_ASSERT_TRUE(fixture.view.contains("Invalid choice."));
}

void test_read_mode_number_parses_digits_and_reports_invalid_empty_or_overflow() {
    UserInputFixture fixture;
    fixture.input.queueLine("");
    fixture.input.queueLine("256");
    fixture.input.queueLine("42");

    TEST_ASSERT_EQUAL_UINT8(0xFF, fixture.manager.readModeNumber());
    TEST_ASSERT_EQUAL_UINT8(0xFF, fixture.manager.readModeNumber());
    TEST_ASSERT_EQUAL_UINT8(42, fixture.manager.readModeNumber());
}

void test_read_mode_number_ignores_non_digits_and_supports_backspace() {
    UserInputFixture fixture;
    fixture.input.blockingChars.push_back(CARDPUTER_SPECIAL_ARROW_UP);
    fixture.input.blockingChars.push_back('1');
    fixture.input.blockingChars.push_back('a');
    fixture.input.blockingChars.push_back('2');
    fixture.input.blockingChars.push_back('\b');
    fixture.input.blockingChars.push_back('3');
    fixture.input.blockingChars.push_back('\n');

    TEST_ASSERT_EQUAL_UINT8(13, fixture.manager.readModeNumber());
    TEST_ASSERT_TRUE(fixture.view.contains(std::string(1, CARDPUTER_SPECIAL_ARROW_UP)));
}

void test_read_validated_pin_number_retries_for_forbidden_gpio() {
    UserInputFixture fixture;
    fixture.input.queueLine("4");
    fixture.input.queueLine("5");

    TEST_ASSERT_EQUAL_UINT8(5, fixture.manager.readValidatedPinNumber("Pin", 1, 0, 10, {4}));
    TEST_ASSERT_TRUE(fixture.view.contains("This GPIO is reserved/protected"));
}

void test_read_validated_pin_group_returns_default_or_retries_protected_pin() {
    UserInputFixture fixture;
    const std::vector<uint8_t> defaultPins = {1, 2};
    const std::vector<uint8_t> protectedPins = {4};

    fixture.input.queueLine("");
    const std::vector<uint8_t> defaultResult =
        fixture.manager.readValidatedPinGroup("Pins", defaultPins, protectedPins);

    TEST_ASSERT_EQUAL_UINT32(2, defaultResult.size());
    TEST_ASSERT_EQUAL_UINT8(1, defaultResult[0]);
    TEST_ASSERT_EQUAL_UINT8(2, defaultResult[1]);

    fixture.input.queueLine("3 4");
    fixture.input.queueLine("5 6");
    const std::vector<uint8_t> selectedPins =
        fixture.manager.readValidatedPinGroup("Pins", defaultPins, protectedPins);

    TEST_ASSERT_EQUAL_UINT32(2, selectedPins.size());
    TEST_ASSERT_EQUAL_UINT8(5, selectedPins[0]);
    TEST_ASSERT_EQUAL_UINT8(6, selectedPins[1]);
    TEST_ASSERT_TRUE(fixture.view.contains("GPIO 4 is protected/reserved."));
}

void test_read_validated_pin_group_retries_invalid_range_and_empty_parse() {
    UserInputFixture fixture;
    fixture.input.queueLine("49");
    fixture.input.queueLine("abc");
    fixture.input.queueLine("7 8");

    const std::vector<uint8_t> selectedPins =
        fixture.manager.readValidatedPinGroup("Pins", {1}, {});

    TEST_ASSERT_EQUAL_UINT32(2, selectedPins.size());
    TEST_ASSERT_EQUAL_UINT8(7, selectedPins[0]);
    TEST_ASSERT_EQUAL_UINT8(8, selectedPins[1]);
    TEST_ASSERT_TRUE(fixture.view.contains("Invalid GPIO: 49"));
    TEST_ASSERT_TRUE(fixture.view.contains("Please enter valid, non-protected GPIOs separated by spaces."));
}

void test_read_validated_hex_string_handles_empty_invalid_length_and_spacing() {
    UserInputFixture fixture;
    fixture.input.queueLine("");
    fixture.input.queueLine("GG");
    fixture.input.queueLine("ABC");
    fixture.input.queueLine("A B C D");
    fixture.input.queueLine("");
    fixture.input.queueLine("");
    fixture.input.queueLine("ABC");
    fixture.input.queueLine("ABCD");

    TEST_ASSERT_EQUAL_STRING("AB CD",
                             fixture.manager.readValidatedHexString("Key", 2).c_str());
    TEST_ASSERT_EQUAL_STRING("00",
                             fixture.manager.readValidatedHexString("Key", 0, true).c_str());
    TEST_ASSERT_EQUAL_STRING("0000",
                             fixture.manager.readValidatedHexString("Key", 0, true, 4).c_str());
    TEST_ASSERT_EQUAL_STRING("ABCD",
                             fixture.manager.readValidatedHexString("Key", 0, true, 4).c_str());
    TEST_ASSERT_TRUE(fixture.view.contains("Input cannot be empty"));
    TEST_ASSERT_TRUE(fixture.view.contains("Invalid characters"));
    TEST_ASSERT_TRUE(fixture.view.contains("Invalid length"));
    TEST_ASSERT_TRUE(fixture.view.contains("multiple of 4"));
}

void test_read_validated_can_id_accepts_default_prefix_and_retries_invalid_values() {
    UserInputFixture fixture;
    fixture.input.queueLine("");
    fixture.input.queueLine("GG");
    fixture.input.queueLine("1234");
    fixture.input.queueLine("800");
    fixture.input.queueLine("0x7FE");

    TEST_ASSERT_EQUAL_HEX16(0x123, fixture.manager.readValidatedCanId("CAN", 0x123));
    TEST_ASSERT_EQUAL_HEX16(0x7FE, fixture.manager.readValidatedCanId("CAN", 0));
    TEST_ASSERT_TRUE(fixture.view.contains("Invalid characters"));
    TEST_ASSERT_TRUE(fixture.view.contains("Too long"));
    TEST_ASSERT_TRUE(fixture.view.contains("exceeds standard 11-bit CAN ID"));
}

void test_read_validated_choice_index_variants_use_default_and_validate_range() {
    UserInputFixture fixture;
    fixture.input.queueLine("");
    fixture.input.queueLine("9");
    fixture.input.queueLine("2");
    fixture.input.queueLine("2");
    fixture.input.queueLine("1");

    const std::vector<std::string> stringChoices = {"alpha", "beta"};
    TEST_ASSERT_EQUAL_INT(1, fixture.manager.readValidatedChoiceIndex("Choice", stringChoices, 1));
    TEST_ASSERT_EQUAL_INT(0, fixture.manager.readValidatedChoiceIndex("Choice", stringChoices, 0));
    TEST_ASSERT_EQUAL_INT(1, fixture.manager.readValidatedChoiceIndex("Choice", stringChoices, 0));
    TEST_ASSERT_EQUAL_INT(1, fixture.manager.readValidatedChoiceIndex("Choice", std::vector<int>{10, 20}, 0));
    TEST_ASSERT_EQUAL_INT(0, fixture.manager.readValidatedChoiceIndex("Choice", std::vector<float>{1.5f, 2.5f}, 1));
    TEST_ASSERT_TRUE(fixture.view.contains("Invalid choice. Using default."));
}

void test_read_validated_char_pointer_choices_handles_missing_and_default_bounds() {
    UserInputFixture fixture;
    const char* const choices[] = {"one", nullptr, "three"};
    fixture.input.queueLine("");
    fixture.input.queueLine("3");
    fixture.input.queueLine("9");

    TEST_ASSERT_EQUAL_INT(-1, fixture.manager.readValidatedChoiceIndex("Choice", nullptr, 0, 0));
    TEST_ASSERT_EQUAL_INT(0, fixture.manager.readValidatedChoiceIndex("Choice", choices, 3, 99));
    TEST_ASSERT_EQUAL_INT(2, fixture.manager.readValidatedChoiceIndex("Choice", choices, 3, 0));
    TEST_ASSERT_EQUAL_INT(0, fixture.manager.readValidatedChoiceIndex("Choice", choices, 3, 0));
    TEST_ASSERT_TRUE(fixture.view.contains("No choices available."));
    TEST_ASSERT_TRUE(fixture.view.contains("Invalid choice. Using default."));
}

void test_read_validated_float_retries_invalid_and_range_then_accepts_trimmed_value() {
    UserInputFixture fixture;
    fixture.input.queueLine("");
    fixture.input.queueLine("abc");
    fixture.input.queueLine("12");
    fixture.input.queueLine(" 3.5 ");

    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.25f, fixture.manager.readValidatedFloat("Float", 1.25f, 0.0f, 10.0f));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.5f, fixture.manager.readValidatedFloat("Float", 0.0f, 0.0f, 10.0f));
    TEST_ASSERT_TRUE(fixture.view.contains("Invalid input. Must be 0.000000 .. 10.000000"));
}

void test_read_validated_numeric_code_retries_length_format_and_swaps_bounds() {
    UserInputFixture fixture;
    fixture.input.queueLine("12");
    fixture.input.queueLine("12AB");
    fixture.input.queueLine("1234");
    fixture.input.queueLine("123");

    TEST_ASSERT_EQUAL_STRING("1234",
                             fixture.manager.readValidatedNumericCode("PIN", "0000", 4, 6).c_str());
    TEST_ASSERT_EQUAL_STRING("123",
                             fixture.manager.readValidatedNumericCode("PIN", "", 5, 3).c_str());
    TEST_ASSERT_TRUE(fixture.view.contains("Invalid length. Expected 4 to 6 digits."));
    TEST_ASSERT_TRUE(fixture.view.contains("Invalid format. Digits only"));
}

}  // namespace user_input_manager_tests

void runUserInputManagerTests() {
    using namespace user_input_manager_tests;
    RUN_TEST(test_get_line_supports_cursor_insertion);
    RUN_TEST(test_get_line_only_number_filters_non_digits);
    RUN_TEST(test_get_line_supports_backspace_at_cursor_and_length_limit);
    RUN_TEST(test_read_string_returns_default_on_empty_or_user_value);
    RUN_TEST(test_read_sanitized_string_filters_or_retries_until_content_remains);
    RUN_TEST(test_read_validated_phone_number_retries_format_length_and_accepts_empty);
    RUN_TEST(test_read_yes_no_uses_default_and_retries_invalid_answer);
    RUN_TEST(test_read_validated_uint8_accepts_hex_and_retries_out_of_range);
    RUN_TEST(test_read_validated_uint16_and_uint32_accept_hex_defaults_and_retries_invalid);
    RUN_TEST(test_read_validated_int_hex_and_byte_retry_invalid_and_range_errors);
    RUN_TEST(test_read_char_choice_uses_default_uppercases_and_retries_invalid);
    RUN_TEST(test_read_mode_number_parses_digits_and_reports_invalid_empty_or_overflow);
    RUN_TEST(test_read_mode_number_ignores_non_digits_and_supports_backspace);
    RUN_TEST(test_read_validated_pin_number_retries_for_forbidden_gpio);
    RUN_TEST(test_read_validated_pin_group_returns_default_or_retries_protected_pin);
    RUN_TEST(test_read_validated_pin_group_retries_invalid_range_and_empty_parse);
    RUN_TEST(test_read_validated_hex_string_handles_empty_invalid_length_and_spacing);
    RUN_TEST(test_read_validated_can_id_accepts_default_prefix_and_retries_invalid_values);
    RUN_TEST(test_read_validated_choice_index_variants_use_default_and_validate_range);
    RUN_TEST(test_read_validated_char_pointer_choices_handles_missing_and_default_bounds);
    RUN_TEST(test_read_validated_float_retries_invalid_and_range_then_accepts_trimmed_value);
    RUN_TEST(test_read_validated_numeric_code_retries_length_format_and_swaps_bounds);
}
