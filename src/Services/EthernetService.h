#pragma once
#include <array>
#include <string>
#include <SPI.h>
#include <ETH.h>
#include "Interfaces/IEthernetService.h"

class EthernetService : public IEthernetService {
public:
  EthernetService();

  bool configure(int8_t pinCS,
                 int8_t pinRST,
                 int8_t pinSCK,
                 int8_t pinMISO,
                 int8_t pinMOSI,
                 int8_t pinIRQ,
                 uint32_t spiHz,
                 const std::array<uint8_t,6>& chosenMac,
                 SPIClass* spi = &SPI,
                 uint8_t phyAddr = 1) override;

  bool beginDHCP(unsigned long timeoutMs) override;

  bool isConnected() const override;
  bool linkUp() const override;

  std::string getMac() const override;
  std::string getLocalIP() const override;
  std::string getSubnetMask() const override;
  std::string getGatewayIp() const override;
  std::string getDns() const override;

  void hardReset() override;
  
private:
  static void onNetEvent(arduino_event_id_t event, arduino_event_info_t info);
  static EthernetService* s_self;
  SPIClass* _spi;
  int8_t _pinCS, _pinRST, _pinSCK, _pinMISO, _pinMOSI, _pinIRQ;
  uint32_t _spiHz;
  uint8_t _phyAddr;
  std::array<uint8_t,6> _mac;

  volatile bool _configured;
  volatile bool _linkUp;
  volatile bool _gotIP;
};
