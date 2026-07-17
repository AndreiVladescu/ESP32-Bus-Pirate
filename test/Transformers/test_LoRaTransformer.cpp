#include <unity.h>

#include "Transformers/LoRaTransformer.h"
#include "../Services/FakeUtilityService.h"

namespace lora_transformer_tests {

void test_transform_accepts_text_and_decodes_common_escapes() {
    FakeUtilityService utility;
    LoRaTransformer transformer{utility};
    std::vector<uint8_t> payload;

    TEST_ASSERT_TRUE(transformer.transform("ping\\n", payload));

    const uint8_t expected[] = {'p', 'i', 'n', 'g', '\n'};
    TEST_ASSERT_EQUAL_UINT32(sizeof(expected), payload.size());
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, payload.data(), sizeof(expected));
}

void test_transform_accepts_hex_payload_and_wildcards() {
    FakeUtilityService utility;
    utility.randomRangeValue = 0xAB;
    LoRaTransformer transformer{utility};
    std::vector<uint8_t> payload;

    TEST_ASSERT_TRUE(transformer.transform("hex{ 01 0x0A ?? FF }", payload));

    const uint8_t expected[] = {0x01, 0x0A, 0xAB, 0xFF};
    TEST_ASSERT_EQUAL_UINT32(sizeof(expected), payload.size());
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, payload.data(), sizeof(expected));
}

void test_transform_rejects_malformed_hex_payloads() {
    FakeUtilityService utility;
    LoRaTransformer transformer{utility};
    std::vector<uint8_t> payload;

    TEST_ASSERT_FALSE(transformer.transform("hex{ 1 GG }", payload));
    TEST_ASSERT_TRUE(payload.empty());
}

void test_file_format_roundtrips_complete_lora_frame() {
    FakeUtilityService utility;
    LoRaTransformer transformer{utility};
    LoRaFrame frame;
    frame.payload = {0xDE, 0xAD, 0xBE, 0xEF};
    frame.profile.frequency = 915.5f;
    frame.profile.bandwidth = 250;
    frame.profile.spreadingFactor = 10;
    frame.profile.codingRate = 6;
    frame.profile.power = 17;
    frame.profile.preambleLength = 12;
    frame.profile.syncWord = 0x3444;
    frame.profile.tcxoVoltage = 2.4f;
    frame.profile.crc = false;
    frame.profile.invertIq = true;
    frame.rssi = -72.5f;
    frame.snr = 8.0f;

    const std::string text = transformer.transformToFileFormat(frame);
    LoRaFrame parsed;

    TEST_ASSERT_TRUE(transformer.transformFromFileFormat(text, parsed));
    TEST_ASSERT_EQUAL_UINT32(frame.payload.size(), parsed.payload.size());
    TEST_ASSERT_EQUAL_UINT8_ARRAY(frame.payload.data(), parsed.payload.data(), frame.payload.size());
    TEST_ASSERT_FLOAT_WITHIN(0.001f, frame.profile.frequency, parsed.profile.frequency);
    TEST_ASSERT_EQUAL_UINT16(frame.profile.bandwidth, parsed.profile.bandwidth);
    TEST_ASSERT_EQUAL_UINT8(frame.profile.spreadingFactor, parsed.profile.spreadingFactor);
    TEST_ASSERT_EQUAL_UINT8(frame.profile.codingRate, parsed.profile.codingRate);
    TEST_ASSERT_EQUAL_INT8(frame.profile.power, parsed.profile.power);
    TEST_ASSERT_EQUAL_UINT16(frame.profile.preambleLength, parsed.profile.preambleLength);
    TEST_ASSERT_EQUAL_HEX16(frame.profile.syncWord, parsed.profile.syncWord);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, frame.profile.tcxoVoltage, parsed.profile.tcxoVoltage);
    TEST_ASSERT_FALSE(parsed.profile.crc);
    TEST_ASSERT_TRUE(parsed.profile.invertIq);
}

void test_file_format_rejects_wrong_payload_length() {
    FakeUtilityService utility;
    LoRaTransformer transformer{utility};
    LoRaFrame parsed;

    const std::string text =
        "Filetype: ESP32-Bit-Pirate LoRa\n"
        "Version: 1\n"
        "FrequencyMHz: 868.000000\n"
        "BandwidthKHz: 125\n"
        "SpreadingFactor: 9\n"
        "CodingRate: 7\n"
        "PowerDbm: 14\n"
        "Preamble: 8\n"
        "SyncWord: 0x1424\n"
        "TcxoVoltage: 1.8\n"
        "CRC: 1\n"
        "InvertIQ: 0\n"
        "RSSI: -80.0\n"
        "SNR: 7.0\n"
        "PayloadLength: 3\n"
        "PayloadHex: AA BB\n";

    TEST_ASSERT_FALSE(transformer.transformFromFileFormat(text, parsed));
}

}  // namespace lora_transformer_tests

void runLoRaTransformerTests() {
    using namespace lora_transformer_tests;
    RUN_TEST(test_transform_accepts_text_and_decodes_common_escapes);
    RUN_TEST(test_transform_accepts_hex_payload_and_wildcards);
    RUN_TEST(test_transform_rejects_malformed_hex_payloads);
    RUN_TEST(test_file_format_roundtrips_complete_lora_frame);
    RUN_TEST(test_file_format_rejects_wrong_payload_length);
}
