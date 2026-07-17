#include <unity.h>

#include "Transformers/AtTransformer.h"

namespace at_transformer_tests {

void test_clean_removes_echo_status_and_trims_meaningful_lines() {
    AtTransformer transformer;

    const std::string cleaned = transformer.clean("\r\nAT+CSQ\r\n  +CSQ: 15,0  \r\nOK\r\n");

    TEST_ASSERT_EQUAL_STRING("+CSQ: 15,0", cleaned.c_str());
}

void test_status_detection_accepts_ok_cme_and_cms_errors() {
    AtTransformer transformer;

    TEST_ASSERT_TRUE(transformer.isOk("\r\nOK\r\n"));
    TEST_ASSERT_FALSE(transformer.isOk("\r\nERROR\r\n"));
    TEST_ASSERT_TRUE(transformer.isError("\r\n+CME ERROR: operation not allowed\r\n"));
    TEST_ASSERT_TRUE(transformer.isError("\r\n+CMS ERROR: 500\r\n"));
}

void test_signal_formats_rssi_and_unknown_values() {
    AtTransformer transformer;

    TEST_ASSERT_EQUAL_STRING("Signal: rssi=15 (-83 dBm), ber=0",
                             transformer.formatSignal("+CSQ: 15,0\r\nOK\r\n").c_str());
    TEST_ASSERT_EQUAL_STRING("Signal: rssi=99 (unknown), ber=7",
                             transformer.formatSignal("+CSQ: 99,7\r\nOK\r\n").c_str());
}

void test_operator_scan_extracts_operator_tuples_and_ignores_capabilities() {
    AtTransformer transformer;
    const std::string raw =
        "+COPS: (2,\"Orange F\",\"Orange\",\"20801\"),"
        "(1,\"Free\",\"Free\",\"20815\"),,(0-4),(0-2)\r\nOK\r\n";

    const std::string formatted = transformer.formatScanOperators(raw);

    TEST_ASSERT_TRUE(formatted.find("Orange F (20801)") != std::string::npos);
    TEST_ASSERT_TRUE(formatted.find("Free (20815)") != std::string::npos);
    TEST_ASSERT_TRUE(formatted.find("0-4") == std::string::npos);
}

void test_sms_read_decodes_hex_payload_line_for_readability() {
    AtTransformer transformer;
    const std::string raw =
        "+CMGR: \"REC READ\",\"+33600000000\",,\"26/07/16,10:30:00+08\"\r\n"
        "48656C6C6F\r\n"
        "OK\r\n";

    const std::string formatted = transformer.formatSmsRead(raw);

    TEST_ASSERT_TRUE(formatted.find("48656C6C6F") != std::string::npos);
    TEST_ASSERT_TRUE(formatted.find("Hello") != std::string::npos);
}

void test_registration_helpers_translate_common_status_codes() {
    AtTransformer transformer;

    TEST_ASSERT_EQUAL_STRING("CS reg: registered (home)",
                             transformer.formatRegistrationCS("+CREG: 0,1\r\nOK\r\n").c_str());
    TEST_ASSERT_EQUAL_STRING("PS reg: registered (roaming)",
                             transformer.formatRegistrationPS("+CGREG: 0,5\r\nOK\r\n").c_str());
}

void test_sim_and_identity_formatters_return_clear_fallbacks() {
    AtTransformer transformer;

    TEST_ASSERT_EQUAL_STRING("SIM state: READY",
                             transformer.formatSimState("AT+CPIN?\r\n+CPIN: READY\r\nOK\r\n").c_str());
    TEST_ASSERT_EQUAL_STRING("IMEI: 123456789012345",
                             transformer.formatImei("\r\n123456789012345\r\nOK\r\n").c_str());
    TEST_ASSERT_EQUAL_STRING("ICCID: no response",
                             transformer.formatIccid("").c_str());
}

}  // namespace at_transformer_tests

void runAtTransformerTests() {
    using namespace at_transformer_tests;
    RUN_TEST(test_clean_removes_echo_status_and_trims_meaningful_lines);
    RUN_TEST(test_status_detection_accepts_ok_cme_and_cms_errors);
    RUN_TEST(test_signal_formats_rssi_and_unknown_values);
    RUN_TEST(test_operator_scan_extracts_operator_tuples_and_ignores_capabilities);
    RUN_TEST(test_sms_read_decodes_hex_payload_line_for_readability);
    RUN_TEST(test_registration_helpers_translate_common_status_codes);
    RUN_TEST(test_sim_and_identity_formatters_return_clear_fallbacks);
}
