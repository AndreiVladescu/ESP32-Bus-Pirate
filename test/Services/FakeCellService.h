#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Interfaces/ICellService.h"

class FakeCellService final : public ICellService {
public:
    struct InitCall {
        uint8_t rxPin = 0;
        uint8_t txPin = 0;
        uint32_t baudrate = 0;
    };

    std::vector<InitCall> initCalls;
    bool detectResult = true;
    bool simReady = true;
    bool pukRequired = false;
    bool enterPinResult = true;
    bool enterPukResult = true;
    bool setOperatorAutoResult = true;
    bool setFunctionalityResult = true;
    bool ussdResult = true;

    std::string moduleInfo = "Quectel EC25\r\nOK\r\n";
    std::string manufacturer = "Quectel\r\nOK\r\n";
    std::string model = "EC25\r\nOK\r\n";
    std::string revision = "Revision: 1.0\r\nOK\r\n";
    std::string imei = "123456789012345\r\nOK\r\n";
    std::string clock = "+CCLK: \"26/07/16,10:30:00+08\"\r\nOK\r\n";
    std::string simState = "+CPIN: READY\r\nOK\r\n";
    std::string iccid = "8986000000000000000\r\nOK\r\n";
    std::string imsi = "208010123456789\r\nOK\r\n";
    std::string msisdn = "+CNUM: ,\"+33600000000\",145\r\nOK\r\n";
    std::string pinLockStatus = "+CLCK: 0\r\nOK\r\n";
    std::string simRetries = "+CPINR: SIM PIN,3\r\nOK\r\n";
    std::string serviceProviderName = "+CSPN: \"Operator\"\r\nOK\r\n";
    std::string phonebookStorage = "+CPBS: \"SM\",1,250\r\nOK\r\n";
    std::string phonebookCaps = "+CPBR: (1-250),40,20\r\nOK\r\n";
    std::string smsStorage = "+CPMS: \"SM\",0,50,\"SM\",0,50,\"SM\",0,50\r\nOK\r\n";
    std::string signal = "+CSQ: 15,0\r\nOK\r\n";
    std::string operatorName = "+COPS: 0,0,\"Orange F\",7\r\nOK\r\n";
    std::string operatorScan = "+COPS: (2,\"Orange F\",\"Orange\",\"20801\")\r\nOK\r\n";
    std::string registrationCS = "+CREG: 0,1\r\nOK\r\n";
    std::string registrationPS = "+CGREG: 0,5\r\nOK\r\n";
    std::string functionality = "+CFUN: 1\r\nOK\r\n";
    std::string attach = "+CGATT: 1\r\nOK\r\n";
    std::string pdpContexts = "+CGDCONT: 1,\"IP\",\"internet\"\r\nOK\r\n";
    std::string pdpActive = "+CGACT: 1,1\r\nOK\r\n";
    std::string phonebookEntries = "+CPBR: 1,\"+33600000000\",145,\"Alice\"\r\nOK\r\n";

    std::string lastPin;
    std::string lastPuk;
    std::string lastNewPin;
    std::string lastUssdCode;
    uint8_t lastUssdDcs = 0;
    uint8_t lastFunctionality = 0;
    uint32_t detectCalls = 0;
    uint32_t setOperatorAutoCalls = 0;
    uint32_t phonebookReadRangeCalls = 0;
    uint16_t lastPhonebookStart = 0;
    uint16_t lastPhonebookEnd = 0;

    void init(uint8_t rxPin, uint8_t txPin, uint32_t baudrate) override {
        initCalls.push_back({rxPin, txPin, baudrate});
    }

    bool detect() override {
        ++detectCalls;
        return detectResult;
    }

    std::string getModuleInfo() override { return moduleInfo; }
    std::string getManufacturer() override { return manufacturer; }
    std::string getModel() override { return model; }
    std::string getRevision() override { return revision; }
    std::string getImei() override { return imei; }
    std::string getClock() override { return clock; }

    bool isSimReady() override { return simReady; }
    std::string getSimState() override { return simState; }

    bool enterPin(const std::string& pin) override {
        lastPin = pin;
        return enterPinResult;
    }

    bool isSimPukRequired() override { return pukRequired; }

    bool enterPuk(const std::string& puk, const std::string& newPin) override {
        lastPuk = puk;
        lastNewPin = newPin;
        return enterPukResult;
    }

    std::string getIccid() override { return iccid; }
    std::string getImsi() override { return imsi; }
    std::string getMsisdn() override { return msisdn; }
    std::string getPinLockStatus() override { return pinLockStatus; }
    std::string getSimRetries() override { return simRetries; }
    std::string getServiceProviderName() override { return serviceProviderName; }
    std::string getPhonebookStorage() override { return phonebookStorage; }
    std::string getPhonebookCaps() override { return phonebookCaps; }
    std::string getSmsStorage() override { return smsStorage; }

    std::string getSignal() override { return signal; }
    std::string getOperator() override { return operatorName; }
    std::string scanOperators(uint32_t = 60000) override { return operatorScan; }

    bool setOperatorAuto() override {
        ++setOperatorAutoCalls;
        return setOperatorAutoResult;
    }

    bool setOperator(const std::string&) override { return false; }
    std::string getRegistrationCS() override { return registrationCS; }
    std::string getRegistrationPS() override { return registrationPS; }

    bool setFunctionality(uint8_t fun) override {
        lastFunctionality = fun;
        return setFunctionalityResult;
    }

    std::string getFunctionality() override { return functionality; }
    bool reboot() override { return false; }

    std::string getAttach() override { return attach; }
    bool setAttach(bool) override { return false; }
    bool definePdpContext(uint8_t, const std::string&, const std::string&) override { return false; }
    std::string queryPdpContexts() override { return pdpContexts; }
    bool activatePdp(uint8_t, bool) override { return false; }
    std::string queryPdpActive() override { return pdpActive; }
    std::string getPdpAddress(uint8_t) override { return ""; }

    bool smsSetTextMode(bool) override { return false; }
    bool smsSetCharset(const std::string&) override { return false; }
    bool smsSetNewIndications(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t) override { return false; }
    std::string smsGetServiceCenter() override { return ""; }
    std::string smsList(const std::string&) override { return ""; }
    std::string smsRead(uint16_t) override { return ""; }
    bool smsDelete(uint16_t, uint8_t) override { return false; }
    bool smsBeginSend(const std::string&) override { return false; }
    bool smsSendText(const std::string&) override { return false; }
    std::string phonebookReadIndex(uint16_t) override { return ""; }

    std::string phonebookReadRange(uint16_t start, uint16_t end) override {
        ++phonebookReadRangeCalls;
        lastPhonebookStart = start;
        lastPhonebookEnd = end;
        return phonebookEntries;
    }

    bool ussdRequest(const std::string& code, uint8_t dcs = 15) override {
        lastUssdCode = code;
        lastUssdDcs = dcs;
        return ussdResult;
    }

    bool ussdCancel() override { return false; }
    bool dial(const std::string&) override { return false; }
    bool answerCall() override { return false; }
    bool hangupCall() override { return false; }
    std::string listCalls() override { return ""; }
    std::string getGsmLocation() override { return ""; }
};
