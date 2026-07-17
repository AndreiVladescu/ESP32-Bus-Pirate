#include <unity.h>

#include "Controllers/RfidController.h"
#include "../Inputs/FakeInput.h"
#include "../Services/FakeRfidService.h"
#include "../Services/FakeUtilityService.h"
#include "../Views/FakeTerminalView.h"

namespace rfid_controller_tests {

struct RfidControllerFixture {
    FakeTerminalView view;
    FakeInput input;
    FakeUtilityService utility;
    FakeRfidService rfidService;
    ArgTransformer transformer;
    UserInputManager userInput{view, input, transformer};
    HelpShell helpShell{view, input, userInput};
    RfidController controller{
        view,
        input,
        utility,
        rfidService,
        userInput,
        transformer,
        helpShell
    };

    RfidControllerFixture() {
        auto& state = GlobalState::getInstance();
        state.setCurrentMode(ModeEnum::RFID);
        state.setRfidSdaPin(1);
        state.setRfidSclPin(2);
    }

    void queueValidUidWriteInputs() {
        input.queueLine("");
        input.queueLine("01020304");
        input.queueLine("08");
        input.queueLine("0004");
    }
};

void test_rfid_config_initializes_service_with_selected_pins() {
    RfidControllerFixture fixture;
    fixture.input.queueLine("5");
    fixture.input.queueLine("6");

    fixture.controller.handleCommand(TerminalCommand("config"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.rfidService.configureCalls);
    TEST_ASSERT_EQUAL_UINT8(5, fixture.rfidService.lastConfiguration.sda);
    TEST_ASSERT_EQUAL_UINT8(6, fixture.rfidService.lastConfiguration.scl);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.rfidService.beginCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("module detected and initialized"));
}

void test_rfid_config_reports_initialization_failure() {
    RfidControllerFixture fixture;
    fixture.rfidService.beginResult = false;
    fixture.input.queueLine("");
    fixture.input.queueLine("");

    fixture.controller.handleCommand(TerminalCommand("config"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.rfidService.beginCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("failed to initialize"));
}

void test_rfid_ensure_configured_retries_after_failure() {
    RfidControllerFixture fixture;
    fixture.rfidService.beginResult = false;
    for (int i = 0; i < 4; ++i) fixture.input.queueLine("");

    fixture.controller.ensureConfigured();
    fixture.controller.ensureConfigured();

    TEST_ASSERT_EQUAL_UINT32(2, fixture.rfidService.configureCalls);
    TEST_ASSERT_EQUAL_UINT32(2, fixture.rfidService.beginCalls);
}

void test_rfid_ensure_configured_reapplies_after_success_without_prompting() {
    RfidControllerFixture fixture;
    fixture.input.queueLine("");
    fixture.input.queueLine("");

    fixture.controller.ensureConfigured();
    fixture.controller.ensureConfigured();

    TEST_ASSERT_EQUAL_UINT32(2, fixture.rfidService.configureCalls);
    TEST_ASSERT_EQUAL_UINT32(2, fixture.rfidService.beginCalls);
    TEST_ASSERT_TRUE(fixture.input.blockingChars.empty());
}

void test_rfid_read_stops_without_polling_when_enter_is_pressed() {
    RfidControllerFixture fixture;
    fixture.input.queueLine("");

    fixture.controller.handleCommand(TerminalCommand("read"));

    TEST_ASSERT_EQUAL_UINT32(0, fixture.rfidService.readCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("RFID Read: Done"));
}

void test_rfid_read_displays_detected_tag_and_selected_mode() {
    RfidControllerFixture fixture;
    fixture.rfidService.readResult = RfidResult::Success;
    fixture.input.queueLine("2");
    fixture.input.queueReadChar('\0');
    fixture.input.queueReadChar('\n');
    fixture.utility.queueNowMs(300);

    fixture.controller.handleCommand(TerminalCommand("read"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.rfidService.readCalls);
    TEST_ASSERT_EQUAL_INT(1, fixture.rfidService.lastReadMode);
    TEST_ASSERT_TRUE(fixture.view.contains("04 A2 1B 00"));
    TEST_ASSERT_TRUE(fixture.view.contains("MIFARE 1K"));
}

void test_rfid_write_uid_rejects_unsupported_uid_length() {
    RfidControllerFixture fixture;
    fixture.input.queueLine("");
    fixture.input.queueLine("010203");

    fixture.controller.handleCommand(TerminalCommand("write"));

    TEST_ASSERT_EQUAL_UINT32(0, fixture.rfidService.cloneCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("Invalid UID length"));
}

void test_rfid_write_uid_sets_fields_parses_and_clones_without_sak_check() {
    RfidControllerFixture fixture;
    fixture.rfidService.cloneResult = RfidResult::Success;
    fixture.queueValidUidWriteInputs();
    fixture.input.queueReadChar('\0');

    fixture.controller.handleCommand(TerminalCommand("write"));

    TEST_ASSERT_EQUAL_STRING("01020304", fixture.rfidService.uidSet.c_str());
    TEST_ASSERT_EQUAL_STRING("08", fixture.rfidService.sakSet.c_str());
    TEST_ASSERT_EQUAL_STRING("00 04", fixture.rfidService.atqaSet.c_str());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.rfidService.parseDataCalls);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.rfidService.cloneCalls);
    TEST_ASSERT_FALSE(fixture.rfidService.lastCloneChecksSak);
    TEST_ASSERT_TRUE(fixture.view.contains("RFID Write UID: Done"));
}

void test_rfid_write_uid_reports_service_error() {
    RfidControllerFixture fixture;
    fixture.rfidService.cloneResult = RfidResult::AuthenticationError;
    fixture.rfidService.failureMessage = "Authentication failed";
    fixture.queueValidUidWriteInputs();
    fixture.input.queueReadChar('\0');

    fixture.controller.handleCommand(TerminalCommand("write"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.rfidService.cloneCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("Authentication failed"));
    TEST_ASSERT_TRUE(fixture.view.contains("block 0 is rewritable"));
}

void test_rfid_write_classic_block_builds_sixteen_byte_dump() {
    RfidControllerFixture fixture;
    fixture.rfidService.writeResult = RfidResult::Success;
    fixture.input.queueLine("2");
    fixture.input.queueLine("");
    fixture.input.queueLine("");
    fixture.input.queueLine("7");
    fixture.input.queueLine("000102030405060708090A0B0C0D0E0F");
    fixture.input.queueReadChar('\0');

    fixture.controller.handleCommand(TerminalCommand("write"));

    TEST_ASSERT_EQUAL_INT(0, fixture.rfidService.lastWriteMode);
    TEST_ASSERT_EQUAL_STRING(
        "Page 7: 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n",
        fixture.rfidService.loadedDump.c_str());
    TEST_ASSERT_TRUE(fixture.view.contains("RFID Write: Done"));
}

void test_rfid_write_ntag_page_builds_four_byte_dump() {
    RfidControllerFixture fixture;
    fixture.rfidService.writeResult = RfidResult::Success;
    fixture.input.queueLine("2");
    fixture.input.queueLine("");
    fixture.input.queueLine("2");
    fixture.input.queueLine("5");
    fixture.input.queueLine("DEADBEEF");
    fixture.input.queueReadChar('\0');

    fixture.controller.handleCommand(TerminalCommand("write"));

    TEST_ASSERT_EQUAL_STRING("Page 5: DE AD BE EF\n", fixture.rfidService.loadedDump.c_str());
    TEST_ASSERT_EQUAL_UINT32(1, fixture.rfidService.writeCalls);
}

void test_rfid_erase_can_be_aborted_before_service_call() {
    RfidControllerFixture fixture;
    fixture.input.queueLine("n");

    fixture.controller.handleCommand(TerminalCommand("erase"));

    TEST_ASSERT_EQUAL_UINT32(0, fixture.rfidService.eraseCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("Aborted"));
}

void test_rfid_erase_reports_success() {
    RfidControllerFixture fixture;
    fixture.rfidService.eraseResult = RfidResult::Success;
    fixture.input.queueLine("y");
    fixture.input.queueReadChar('\0');

    fixture.controller.handleCommand(TerminalCommand("erase"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.rfidService.eraseCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("RFID Erase: Done"));
}

void test_rfid_erase_reports_non_retryable_failure() {
    RfidControllerFixture fixture;
    fixture.rfidService.eraseResult = RfidResult::AuthenticationError;
    fixture.rfidService.failureMessage = "Bad key";
    fixture.input.queueLine("y");
    fixture.input.queueReadChar('\0');

    fixture.controller.handleCommand(TerminalCommand("erase"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.rfidService.eraseCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("Bad key"));
    TEST_ASSERT_TRUE(fixture.view.contains("Failed to erase tag"));
}

void test_rfid_clone_reads_source_then_clones_target() {
    RfidControllerFixture fixture;
    fixture.rfidService.readResult = RfidResult::Success;
    fixture.rfidService.cloneResult = RfidResult::Success;
    fixture.input.queueLine("");
    fixture.input.queueLine("");
    fixture.input.queueReadChar('\0');
    fixture.input.queueReadChar('\0');
    fixture.utility.queueNowMs(300);

    fixture.controller.handleCommand(TerminalCommand("clone"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.rfidService.readCalls);
    TEST_ASSERT_EQUAL_UINT32(1, fixture.rfidService.cloneCalls);
    TEST_ASSERT_TRUE(fixture.rfidService.lastCloneChecksSak);
    TEST_ASSERT_TRUE(fixture.view.contains("RFID UID Clone: Done"));
}

void test_rfid_clone_can_be_cancelled_after_reading_source() {
    RfidControllerFixture fixture;
    fixture.rfidService.readResult = RfidResult::Success;
    fixture.input.queueLine("");
    fixture.input.queueLine("n");
    fixture.input.queueReadChar('\0');
    fixture.utility.queueNowMs(300);

    fixture.controller.handleCommand(TerminalCommand("clone"));

    TEST_ASSERT_EQUAL_UINT32(1, fixture.rfidService.readCalls);
    TEST_ASSERT_EQUAL_UINT32(0, fixture.rfidService.cloneCalls);
    TEST_ASSERT_TRUE(fixture.view.contains("Cancelled by user"));
}

void test_rfid_unknown_command_displays_help() {
    RfidControllerFixture fixture;

    fixture.controller.handleCommand(TerminalCommand("unknown"));

    TEST_ASSERT_TRUE(fixture.view.contains("Unknown command. Available RFID commands"));
    TEST_ASSERT_TRUE(fixture.view.contains("RFID (PN532)"));
    TEST_ASSERT_TRUE(fixture.view.contains("erase"));
}

}  // namespace rfid_controller_tests

void runRfidControllerTests() {
    using namespace rfid_controller_tests;
    RUN_TEST(test_rfid_config_initializes_service_with_selected_pins);
    RUN_TEST(test_rfid_config_reports_initialization_failure);
    RUN_TEST(test_rfid_ensure_configured_retries_after_failure);
    RUN_TEST(test_rfid_ensure_configured_reapplies_after_success_without_prompting);
    RUN_TEST(test_rfid_read_stops_without_polling_when_enter_is_pressed);
    RUN_TEST(test_rfid_read_displays_detected_tag_and_selected_mode);
    RUN_TEST(test_rfid_write_uid_rejects_unsupported_uid_length);
    RUN_TEST(test_rfid_write_uid_sets_fields_parses_and_clones_without_sak_check);
    RUN_TEST(test_rfid_write_uid_reports_service_error);
    RUN_TEST(test_rfid_write_classic_block_builds_sixteen_byte_dump);
    RUN_TEST(test_rfid_write_ntag_page_builds_four_byte_dump);
    RUN_TEST(test_rfid_erase_can_be_aborted_before_service_call);
    RUN_TEST(test_rfid_erase_reports_success);
    RUN_TEST(test_rfid_erase_reports_non_retryable_failure);
    RUN_TEST(test_rfid_clone_reads_source_then_clones_target);
    RUN_TEST(test_rfid_clone_can_be_cancelled_after_reading_source);
    RUN_TEST(test_rfid_unknown_command_displays_help);
}
