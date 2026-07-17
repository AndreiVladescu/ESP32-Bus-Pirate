#include <unity.h>

#include "Transformers/InfraredRemoteTransformer.h"

namespace infrared_remote_transformer_tests {

void test_valid_file_requires_ir_filetype_on_first_line() {
    TEST_ASSERT_TRUE(InfraredRemoteTransformer::isValidInfraredFile("Filetype: IR\nVersion: 1\n"));
    TEST_ASSERT_FALSE(InfraredRemoteTransformer::isValidInfraredFile("Version: 1\nFiletype: IR\n"));
    TEST_ASSERT_FALSE(InfraredRemoteTransformer::isValidInfraredFile(""));
}

void test_parsed_command_imports_little_endian_address_and_one_byte_command() {
    const std::string file =
        "Filetype: IR\n"
        "Version: 1\n\n"
        "name: Power\n"
        "type: parsed\n"
        "protocol: NEC\n"
        "address: 34 12\n"
        "command: AB\n";

    auto commands = InfraredRemoteTransformer::transformFromFileFormat(file);

    TEST_ASSERT_EQUAL_UINT32(1, commands.size());
    TEST_ASSERT_EQUAL_STRING("Power", commands[0].functionName.c_str());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(_NEC), static_cast<int>(commands[0].protocol));
    TEST_ASSERT_EQUAL_HEX16(0x1234, commands[0].address);
    TEST_ASSERT_EQUAL_HEX8(0xAB, commands[0].function);
}

void test_raw_command_imports_frequency_duty_and_data() {
    const std::string file =
        "Filetype: IR\n"
        "Version: 1\n\n"
        "name: RawOne\n"
        "type: raw\n"
        "frequency: 38000\n"
        "duty_cycle: 0.33\n"
        "data: 9000 4500 560\n";

    auto commands = InfraredRemoteTransformer::transformFromFileFormat(file);

    TEST_ASSERT_EQUAL_UINT32(1, commands.size());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(_RAW), static_cast<int>(commands[0].protocol));
    TEST_ASSERT_EQUAL_INT(38, commands[0].frequency);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.33f, commands[0].dutyCycle);
    TEST_ASSERT_EQUAL_UINT32(3, commands[0].rawDataSize);
    TEST_ASSERT_EQUAL_UINT16(9000, commands[0].rawData[0]);
    TEST_ASSERT_EQUAL_UINT16(4500, commands[0].rawData[1]);
    TEST_ASSERT_EQUAL_UINT16(560, commands[0].rawData[2]);

    delete[] commands[0].rawData;
}

void test_transform_to_file_format_writes_parsed_command_with_expected_byte_widths() {
    InfraredFileRemoteCommand cmd{};
    cmd.functionName = "Power";
    cmd.protocol = _NEC;
    cmd.address = 0x1234;
    cmd.function = 0xAB;

    const std::string file = InfraredRemoteTransformer::transformToFileFormat("remote.ir", {cmd});

    TEST_ASSERT_TRUE(file.find("Filetype: IR") != std::string::npos);
    TEST_ASSERT_TRUE(file.find("name: Power") != std::string::npos);
    TEST_ASSERT_TRUE(file.find("address: 34 12") != std::string::npos);
    TEST_ASSERT_TRUE(file.find("command: AB\n") != std::string::npos);
    TEST_ASSERT_TRUE(file.find("command: AB 00") == std::string::npos);
}

void test_transform_to_file_format_writes_raw_command() {
    uint16_t rawData[] = {9000, 4500, 560};
    InfraredFileRemoteCommand cmd{};
    cmd.functionName = "RawOne";
    cmd.protocol = _RAW;
    cmd.frequency = 38;
    cmd.dutyCycle = 0.33f;
    cmd.rawData = rawData;
    cmd.rawDataSize = 3;

    const std::string file = InfraredRemoteTransformer::transformToFileFormat("remote.ir", {cmd});

    TEST_ASSERT_TRUE(file.find("type: raw") != std::string::npos);
    TEST_ASSERT_TRUE(file.find("frequency: 38000") != std::string::npos);
    TEST_ASSERT_TRUE(file.find("duty_cycle: 0.33") != std::string::npos);
    TEST_ASSERT_TRUE(file.find("data: 9000 4500 560") != std::string::npos);
}

void test_extract_function_names_preserves_order() {
    InfraredFileRemoteCommand first{};
    first.functionName = "Power";
    InfraredFileRemoteCommand second{};
    second.functionName = "VolumeUp";

    const auto names = InfraredRemoteTransformer::extractFunctionNames({first, second});

    TEST_ASSERT_EQUAL_UINT32(2, names.size());
    TEST_ASSERT_EQUAL_STRING("Power", names[0].c_str());
    TEST_ASSERT_EQUAL_STRING("VolumeUp", names[1].c_str());
}

}  // namespace infrared_remote_transformer_tests

void runInfraredRemoteTransformerTests() {
    using namespace infrared_remote_transformer_tests;
    RUN_TEST(test_valid_file_requires_ir_filetype_on_first_line);
    RUN_TEST(test_parsed_command_imports_little_endian_address_and_one_byte_command);
    RUN_TEST(test_raw_command_imports_frequency_duty_and_data);
    RUN_TEST(test_transform_to_file_format_writes_parsed_command_with_expected_byte_widths);
    RUN_TEST(test_transform_to_file_format_writes_raw_command);
    RUN_TEST(test_extract_function_names_preserves_order);
}
