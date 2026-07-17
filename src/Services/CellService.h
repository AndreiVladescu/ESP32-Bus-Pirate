#pragma once

#include <string>
#include <stdint.h>
#include "Data/CellAtProfiles.h"
#include "Interfaces/ICellService.h"

class CellService : public ICellService {
public:
    void init(uint8_t rxPin, uint8_t txPin, uint32_t baudrate) override;
    bool detect() override;

    // Basic / identity
    std::string getModuleInfo() override;
    std::string getManufacturer() override;
    std::string getModel() override;
    std::string getRevision() override;
    std::string getImei() override;
    std::string getClock() override;
    
    // SIM
    bool isSimReady() override;
    std::string getSimState() override;
    bool enterPin(const std::string& pin) override;
    bool isSimPukRequired() override;
    bool enterPuk(const std::string& puk, const std::string& newPin) override;
    std::string getIccid() override;
    std::string getImsi() override;
    std::string getMsisdn() override;
    std::string getPinLockStatus() override;
    std::string getSimRetries() override;
    std::string getServiceProviderName() override;
    std::string getPhonebookStorage() override;
    std::string getPhonebookCaps() override;
    std::string getSmsStorage() override;

    // Network
    std::string getSignal() override;
    std::string getOperator() override;
    std::string scanOperators(uint32_t timeoutMs = 60000) override;
    bool setOperatorAuto() override;
    bool setOperator(const std::string& mccmnc) override;
    std::string getRegistrationCS() override;
    std::string getRegistrationPS() override;
    bool setFunctionality(uint8_t fun) override;
    std::string getFunctionality() override;
    bool reboot() override;

    // PDP / attach
    std::string getAttach() override;
    bool setAttach(bool attached) override;
    bool definePdpContext(uint8_t cid, const std::string& pdpType, const std::string& apn) override;
    std::string queryPdpContexts() override;
    bool activatePdp(uint8_t cid, bool active) override;
    std::string queryPdpActive() override;
    std::string getPdpAddress(uint8_t cid) override;

    // SMS
    bool smsSetTextMode(bool enabled) override;
    bool smsSetCharset(const std::string& charset) override;
    bool smsSetNewIndications(uint8_t mode, uint8_t mt, uint8_t bm, uint8_t ds, uint8_t bfr) override;
    std::string smsGetServiceCenter() override;
    std::string smsList(const std::string& filter) override;
    std::string smsRead(uint16_t index) override;
    bool smsDelete(uint16_t index, uint8_t flag) override;
    bool smsBeginSend(const std::string& number) override; // waits for '>'
    bool smsSendText(const std::string& text) override;    // sends text + Ctrl+Z
    std::string phonebookReadIndex(uint16_t index) override;
    std::string phonebookReadRange(uint16_t start, uint16_t end) override;

    // USSD
    bool ussdRequest(const std::string& code, uint8_t dcs = 15) override;
    bool ussdCancel() override;

    // Calls
    bool dial(const std::string& number) override;
    bool answerCall() override;
    bool hangupCall() override;
    std::string listCalls() override;

    // GSM loc
    std::string getGsmLocation() override;

private:
    std::string sendCommand(const std::string& cmd, uint32_t timeoutMs = 1000);
    std::string readResponse(uint32_t timeoutMs);
    void flushInput();
    bool sendExpectOk(const std::string& cmd, uint32_t timeoutMs = 1000);

    // Formatting helpers for profile templates
    std::string format1(const char* fmt, const std::string& a);
    std::string format1u(const char* fmt, uint32_t v);
    std::string format2u(const char* fmt, uint32_t a, uint32_t b);
    std::string format3(const char* fmt, int a, const std::string& b, const std::string& c);
    std::string format5u(const char* fmt, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e);

    bool sendTextAndCtrlZExpectOk(const std::string& text, uint32_t timeoutMs);

    uint32_t _baudrate = 0;
    CellAtProfile _profile = GENERIC_CELL_PROFILE;
};
