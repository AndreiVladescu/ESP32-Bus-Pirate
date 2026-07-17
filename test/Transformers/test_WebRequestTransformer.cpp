#include <unity.h>

#include <string>

#include "Transformers/WebRequestTransformer.h"

namespace web_request_transformer_tests {

void test_json_body_command_field_is_extracted_as_terminal_command() {
    WebRequestTransformer transformer;

    const TerminalCommand command = transformer.toTerminalCommand("{\"command\":\"help\"}");

    TEST_ASSERT_EQUAL_STRING("help", command.getRoot().c_str());
    TEST_ASSERT_EQUAL_STRING("", command.getSubcommand().c_str());
    TEST_ASSERT_EQUAL_STRING("", command.getArgs().c_str());
}

void test_raw_terminal_command_extracts_json_command_or_falls_back_to_body() {
    WebRequestTransformer transformer;

    TEST_ASSERT_EQUAL_STRING("mode i2c",
                             transformer.toTerminalCommandRaw("{\"command\":\"mode i2c\"}").c_str());
    TEST_ASSERT_EQUAL_STRING("plain command",
                             transformer.toTerminalCommandRaw("plain command").c_str());
    TEST_ASSERT_EQUAL_STRING("{\"other\":\"value\"}",
                             transformer.toTerminalCommandRaw("{\"other\":\"value\"}").c_str());
}

void test_to_terminal_command_falls_back_to_raw_body_when_json_has_no_command() {
    WebRequestTransformer transformer;

    const TerminalCommand command = transformer.toTerminalCommand("{\"other\":\"value\"}");

    TEST_ASSERT_EQUAL_STRING("{\"other\":\"value\"}", command.getRoot().c_str());
}

void test_to_json_response_wraps_and_escapes_cli_output() {
    WebRequestTransformer transformer;

    TEST_ASSERT_EQUAL_STRING("{\"result\":\"ok\\n\\\"done\\\"\"}",
                             transformer.toJsonResponse("ok\n\"done\"").c_str());
}

}  // namespace web_request_transformer_tests

void runWebRequestTransformerTests() {
    using namespace web_request_transformer_tests;
    RUN_TEST(test_json_body_command_field_is_extracted_as_terminal_command);
    RUN_TEST(test_raw_terminal_command_extracts_json_command_or_falls_back_to_body);
    RUN_TEST(test_to_terminal_command_falls_back_to_raw_body_when_json_has_no_command);
    RUN_TEST(test_to_json_response_wraps_and_escapes_cli_output);
}
