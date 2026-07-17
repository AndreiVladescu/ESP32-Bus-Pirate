#pragma once

#include <cstdint>
#include <string>

class ICellService {
public:
    virtual ~ICellService() = default;

    virtual void init(uint8_t rxPin, uint8_t txPin, uint32_t baudrate) = 0;
    virtual bool detect() = 0;

    virtual std::string getModuleInfo() = 0;
    virtual std::string getManufacturer() = 0;
    virtual std::string getModel() = 0;
    virtual std::string getRevision() = 0;
    virtual std::string getImei() = 0;
    virtual std::string getClock() = 0;

    virtual bool isSimReady() = 0;
    virtual std::string getSimState() = 0;
    virtual bool enterPin(const std::string& pin) = 0;
    virtual bool isSimPukRequired() = 0;
    virtual bool enterPuk(const std::string& puk, const std::string& newPin) = 0;
    virtual std::string getIccid() = 0;
    virtual std::string getImsi() = 0;
    virtual std::string getMsisdn() = 0;
    virtual std::string getPinLockStatus() = 0;
    virtual std::string getSimRetries() = 0;
    virtual std::string getServiceProviderName() = 0;
    virtual std::string getPhonebookStorage() = 0;
    virtual std::string getPhonebookCaps() = 0;
    virtual std::string getSmsStorage() = 0;

    virtual std::string getSignal() = 0;
    virtual std::string getOperator() = 0;
    virtual std::string scanOperators(uint32_t timeoutMs = 60000) = 0;
    virtual bool setOperatorAuto() = 0;
    virtual bool setOperator(const std::string& mccmnc) = 0;
    virtual std::string getRegistrationCS() = 0;
    virtual std::string getRegistrationPS() = 0;
    virtual bool setFunctionality(uint8_t fun) = 0;
    virtual std::string getFunctionality() = 0;
    virtual bool reboot() = 0;

    virtual std::string getAttach() = 0;
    virtual bool setAttach(bool attached) = 0;
    virtual bool definePdpContext(uint8_t cid, const std::string& pdpType, const std::string& apn) = 0;
    virtual std::string queryPdpContexts() = 0;
    virtual bool activatePdp(uint8_t cid, bool active) = 0;
    virtual std::string queryPdpActive() = 0;
    virtual std::string getPdpAddress(uint8_t cid) = 0;

    virtual bool smsSetTextMode(bool enabled) = 0;
    virtual bool smsSetCharset(const std::string& charset) = 0;
    virtual bool smsSetNewIndications(uint8_t mode, uint8_t mt, uint8_t bm, uint8_t ds, uint8_t bfr) = 0;
    virtual std::string smsGetServiceCenter() = 0;
    virtual std::string smsList(const std::string& filter) = 0;
    virtual std::string smsRead(uint16_t index) = 0;
    virtual bool smsDelete(uint16_t index, uint8_t flag) = 0;
    virtual bool smsBeginSend(const std::string& number) = 0;
    virtual bool smsSendText(const std::string& text) = 0;
    virtual std::string phonebookReadIndex(uint16_t index) = 0;
    virtual std::string phonebookReadRange(uint16_t start, uint16_t end) = 0;

    virtual bool ussdRequest(const std::string& code, uint8_t dcs = 15) = 0;
    virtual bool ussdCancel() = 0;

    virtual bool dial(const std::string& number) = 0;
    virtual bool answerCall() = 0;
    virtual bool hangupCall() = 0;
    virtual std::string listCalls() = 0;

    virtual std::string getGsmLocation() = 0;
};
