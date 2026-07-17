#include <unity.h>

#include <string>
#include <vector>

#include "Transformers/SubGhzTransformer.h"

namespace subghz_transformer_tests {

bool containsText(const std::string& value, const std::string& text) {
    return value.find(text) != std::string::npos;
}

rmt_symbol_word_t symbol(uint32_t duration0, uint32_t level0, uint32_t duration1, uint32_t level1) {
    rmt_symbol_word_t s{};
    s.duration0 = duration0;
    s.level0 = level0;
    s.duration1 = duration1;
    s.level1 = level1;
    return s;
}

void test_validation_accepts_flipper_subghz_header_with_bom() {
    SubGhzTransformer transformer;
    const std::string valid = std::string("\xEF\xBB\xBF") +
        "Filetype: Flipper SubGhz RAW File\n"
        "Version: 1\n";

    TEST_ASSERT_TRUE(transformer.isValidSubGhzFile(valid));
    TEST_ASSERT_FALSE(transformer.isValidSubGhzFile("Filetype: Flipper Infrared File\n"));
    TEST_ASSERT_FALSE(transformer.isValidSubGhzFile(""));
}

void test_parse_raw_file_collects_signed_timings_and_metadata() {
    SubGhzTransformer transformer;
    const std::string content =
        "Filetype: Flipper SubGhz RAW File\n"
        "Version: 1\n"
        "Frequency: 433920000\n"
        "Preset: FuriHalSubGhzPresetOok650Async\n"
        "Protocol: RAW\n"
        "RAW_Data: 350 -1050 350 -350 0\n"
        "RAW_Data: 1050 -350\n";

    const auto commands = transformer.transformFromFileFormat(content, "/captures/gate.sub");

    TEST_ASSERT_EQUAL_UINT32(1, commands.size());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SubGhzProtocolEnum::RAW), static_cast<int>(commands[0].protocol));
    TEST_ASSERT_EQUAL_UINT32(433920000, commands[0].frequency_hz);
    TEST_ASSERT_EQUAL_STRING("FuriHalSubGhzPresetOok650Async", commands[0].preset.c_str());
    TEST_ASSERT_EQUAL_STRING("/captures/gate.sub", commands[0].source_file.c_str());

    const std::vector<int32_t> expected = {350, -1050, 350, -350, 1050, -350};
    TEST_ASSERT_EQUAL_UINT32(expected.size(), commands[0].raw_timings.size());
    TEST_ASSERT_EQUAL_INT32_ARRAY(expected.data(), commands[0].raw_timings.data(), expected.size());
}

void test_parse_binraw_file_uses_data_raw_as_bytes_and_preserves_bit_count() {
    SubGhzTransformer transformer;
    const std::string content =
        "Filetype: Flipper SubGhz Key File\n"
        "Version: 1\n"
        "Frequency: 315000000\n"
        "Preset: FuriHalSubGhzPreset2FSKDev238Async\n"
        "Protocol: BinRAW\n"
        "TE: 400\n"
        "Bit: 20\n"
        "Data_RAW: AA 0F\n"
        "Data_RAW: 55\n";

    const auto commands = transformer.transformFromFileFormat(content);

    TEST_ASSERT_EQUAL_UINT32(1, commands.size());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SubGhzProtocolEnum::BinRAW), static_cast<int>(commands[0].protocol));
    TEST_ASSERT_EQUAL_UINT32(315000000, commands[0].frequency_hz);
    TEST_ASSERT_EQUAL_UINT16(400, commands[0].te_us);
    TEST_ASSERT_EQUAL_UINT16(20, commands[0].bits);

    const uint8_t expected[] = {0xAA, 0x0F, 0x55};
    TEST_ASSERT_EQUAL_UINT32(sizeof(expected), commands[0].bitstream_bytes.size());
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, commands[0].bitstream_bytes.data(), sizeof(expected));
}

void test_parse_key_file_creates_one_command_per_key() {
    SubGhzTransformer transformer;
    const std::string content =
        "Filetype: Flipper SubGhz Key File\n"
        "Version: 1\n"
        "Frequency: 433920000\n"
        "Preset: FuriHalSubGhzPresetOok270Async\n"
        "Protocol: Princeton\n"
        "TE: 350\n"
        "Bit: 24\n"
        "Key: 00 00 00 00 00 AB CD EF\n"
        "Key: 00 00 00 00 00 12 34 56\n";

    const auto commands = transformer.transformFromFileFormat(content);

    TEST_ASSERT_EQUAL_UINT32(2, commands.size());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(SubGhzProtocolEnum::Princeton), static_cast<int>(commands[0].protocol));
    TEST_ASSERT_EQUAL_UINT16(24, commands[0].bits);
    TEST_ASSERT_EQUAL_UINT16(350, commands[0].te_us);
    TEST_ASSERT_EQUAL_HEX64(0xABCDEFULL, commands[0].key);
    TEST_ASSERT_EQUAL_HEX64(0x123456ULL, commands[1].key);
}

void test_format_raw_binraw_and_key_files() {
    SubGhzTransformer transformer;

    SubGhzFileCommand raw{};
    raw.protocol = SubGhzProtocolEnum::RAW;
    raw.frequency_hz = 433920000;
    raw.preset = "FuriHalSubGhzPresetOok650Async";
    raw.raw_timings = {100, -200, 300};

    const std::string rawText = transformer.transformToFileFormat(raw);
    TEST_ASSERT_TRUE(containsText(rawText, "Filetype: Flipper SubGhz RAW File"));
    TEST_ASSERT_TRUE(containsText(rawText, "Protocol: RAW"));
    TEST_ASSERT_TRUE(containsText(rawText, "RAW_Data: 100 -200 300"));

    SubGhzFileCommand key{};
    key.protocol = SubGhzProtocolEnum::RcSwitch;
    key.frequency_hz = 433920000;
    key.bits = 24;
    key.te_us = 350;
    key.key = 0xABCDEF;

    const std::string keyText = transformer.transformToFileFormat(key);
    TEST_ASSERT_TRUE(containsText(keyText, "Filetype: Flipper SubGhz Key File"));
    TEST_ASSERT_TRUE(containsText(keyText, "Protocol: RcSwitch"));
    TEST_ASSERT_TRUE(containsText(keyText, "Key: 00 00 00 00 00 AB CD EF"));
}

void test_extract_summaries_describes_command_payloads() {
    SubGhzTransformer transformer;
    SubGhzFileCommand raw{};
    raw.protocol = SubGhzProtocolEnum::RAW;
    raw.frequency_hz = 433920000;
    raw.raw_timings = {100, -200};

    SubGhzFileCommand bin{};
    bin.protocol = SubGhzProtocolEnum::BinRAW;
    bin.frequency_hz = 315000000;
    bin.bitstream_bytes = {0xAA, 0x55, 0x01};

    const auto summaries = transformer.extractSummaries({raw, bin});

    TEST_ASSERT_EQUAL_UINT32(2, summaries.size());
    TEST_ASSERT_TRUE(containsText(summaries[0], "[RAW]"));
    TEST_ASSERT_TRUE(containsText(summaries[0], "timings=2"));
    TEST_ASSERT_TRUE(containsText(summaries[1], "[BinRAW]"));
    TEST_ASSERT_TRUE(containsText(summaries[1], "bytes=3"));
}

void test_symbols_to_signed_timings_respects_levels_and_tick_rate() {
    SubGhzTransformer transformer;
    const std::vector<rmt_symbol_word_t> symbols = {
        symbol(10, 1, 20, 0),
        symbol(0, 1, 5, 1)
    };

    const auto timings = transformer.symbolsToSignedTimings(symbols, 2);

    const std::vector<int32_t> expected = {5, -10, 3};
    TEST_ASSERT_EQUAL_UINT32(expected.size(), timings.size());
    TEST_ASSERT_EQUAL_INT32_ARRAY(expected.data(), timings.data(), expected.size());
}

void test_repeat_frame_adds_low_gaps_between_small_frames() {
    SubGhzTransformer transformer;
    const std::vector<rmt_symbol_word_t> frame = {
        symbol(10, 1, 20, 0)
    };

    const auto repeated = transformer.repeatFrameWithGap(frame, 2, 3, 100);

    TEST_ASSERT_EQUAL_UINT32(5, repeated.size());
    TEST_ASSERT_EQUAL_UINT32(10, repeated[0].duration0);
    TEST_ASSERT_EQUAL_UINT32(200, repeated[1].duration0);
    TEST_ASSERT_EQUAL_UINT32(0, repeated[1].level0);
    TEST_ASSERT_EQUAL_UINT32(10, repeated[2].duration0);
    TEST_ASSERT_EQUAL_UINT32(200, repeated[3].duration0);
    TEST_ASSERT_EQUAL_UINT32(10, repeated[4].duration0);
}

}  // namespace subghz_transformer_tests

void runSubGhzTransformerTests() {
    using namespace subghz_transformer_tests;
    RUN_TEST(test_validation_accepts_flipper_subghz_header_with_bom);
    RUN_TEST(test_parse_raw_file_collects_signed_timings_and_metadata);
    RUN_TEST(test_parse_binraw_file_uses_data_raw_as_bytes_and_preserves_bit_count);
    RUN_TEST(test_parse_key_file_creates_one_command_per_key);
    RUN_TEST(test_format_raw_binraw_and_key_files);
    RUN_TEST(test_extract_summaries_describes_command_payloads);
    RUN_TEST(test_symbols_to_signed_timings_respects_levels_and_tick_rate);
    RUN_TEST(test_repeat_frame_adds_low_gaps_between_small_frames);
}

