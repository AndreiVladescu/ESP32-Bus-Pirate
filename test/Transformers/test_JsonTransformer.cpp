#include <unity.h>

#include <string>
#include <vector>

#include "Transformers/JsonTransformer.h"

namespace json_transformer_tests {

void test_to_lines_pretty_prints_valid_json() {
    JsonTransformer transformer;

    const auto lines = transformer.toLines("{\"name\":\"bus\",\"items\":[1,2]}");

    TEST_ASSERT_FALSE(lines.empty());
    TEST_ASSERT_EQUAL_STRING("{", lines.front().c_str());
    TEST_ASSERT_EQUAL_STRING("}", lines.back().c_str());
    TEST_ASSERT_TRUE(lines[1].find("\"name\"") != std::string::npos);
    TEST_ASSERT_TRUE(lines[2].find("\"items\"") != std::string::npos);
}

void test_to_lines_reports_invalid_json() {
    JsonTransformer transformer;

    const auto lines = transformer.toLines("{not json");

    TEST_ASSERT_EQUAL_UINT32(1, lines.size());
    TEST_ASSERT_EQUAL_STRING("No results found.", lines[0].c_str());
}

void test_dechunk_concatenates_valid_chunks_and_stops_on_zero_or_invalid_input() {
    JsonTransformer transformer;

    TEST_ASSERT_EQUAL_STRING("Wikipedia",
                             transformer.dechunk("4\r\nWiki\r\n5\r\npedia\r\n0\r\n\r\n").c_str());
    TEST_ASSERT_EQUAL_STRING("Wiki",
                             transformer.dechunk("4\r\nWiki\r\nbad\r\nignored").c_str());
    TEST_ASSERT_EQUAL_STRING("",
                             transformer.dechunk("4\r\nWi").c_str());
}

void test_escape_handles_json_specials_and_control_characters() {
    std::string input = "a\"b\\c\n\t";
    input.push_back(static_cast<char>(0x01));

    TEST_ASSERT_EQUAL_STRING("a\\\"b\\\\c\\n\\t\\u0001",
                             JsonTransformer::escape(input).c_str());
}

void test_make_entry_json_escapes_name_and_sets_type() {
    TEST_ASSERT_EQUAL_STRING("{\"name\":\"dir/file\\\".txt\",\"size\":12,\"isDir\":false}",
                             JsonTransformer::makeEntryJson("dir/file\".txt", 12, false).c_str());
    TEST_ASSERT_EQUAL_STRING("{\"name\":\"folder\",\"size\":0,\"isDir\":true}",
                             JsonTransformer::makeEntryJson("folder", 0, true).c_str());
}

void test_make_ls_json_builds_listing_and_rejects_mismatched_vectors() {
    const std::vector<std::string> names = {"one.txt", "sub\"dir"};
    const std::vector<size_t> sizes = {4, 0};
    const std::vector<uint8_t> isDirs = {0, 1};

    TEST_ASSERT_EQUAL_STRING(
        "{\"dir\":\"/tmp\",\"total\":100,\"used\":40,\"entries\":["
        "{\"name\":\"one.txt\",\"size\":4,\"isDir\":false},"
        "{\"name\":\"sub\\\"dir\",\"size\":0,\"isDir\":true}]}",
        JsonTransformer::makeLsJson("/tmp", 100, 40, names, sizes, isDirs).c_str());

    TEST_ASSERT_EQUAL_STRING("{\"error\":\"mismatched vector sizes\"}",
                             JsonTransformer::makeLsJson("/", 0, 0, names, {1}, isDirs).c_str());
}

}  // namespace json_transformer_tests

void runJsonTransformerTests() {
    using namespace json_transformer_tests;
    RUN_TEST(test_to_lines_pretty_prints_valid_json);
    RUN_TEST(test_to_lines_reports_invalid_json);
    RUN_TEST(test_dechunk_concatenates_valid_chunks_and_stops_on_zero_or_invalid_input);
    RUN_TEST(test_escape_handles_json_specials_and_control_characters);
    RUN_TEST(test_make_entry_json_escapes_name_and_sets_type);
    RUN_TEST(test_make_ls_json_builds_listing_and_rejects_mismatched_vectors);
}
