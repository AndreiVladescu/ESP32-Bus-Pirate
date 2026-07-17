#pragma once
#include <cstdint>
#include <string>
#include "Interfaces/ITelnetService.h"

extern "C" {
  #include <lwip/sockets.h>
  #include <lwip/inet.h>
  #include <lwip/netdb.h>
}

class TelnetService : public ITelnetService {
public:
  // Connect to a Telnet server (synchronous)
  bool connectTo(const std::string& host, uint16_t port, uint32_t recvTimeoutMs = 3000) override;

  void close() override;
  bool isConnected() const override { return _sock >= 0; }

  // Send a character
  bool writeChar(char c) override;

  // Send raw data
  int  writeRaw(const char* data, size_t len) override;

  // Send a line
  bool writeLine(const std::string& line) override;

  // Read the socket
  void poll() override;

  // Retrieve and clear the text buffer accumulated by poll()
  std::string readOutputNonBlocking() override;

  // Get the last error message if connection failed
  const std::string& lastError() const override { return _lastError; }

private:
  // Telnet constants
  static constexpr uint8_t IAC   = 255;
  static constexpr uint8_t DONT  = 254;
  static constexpr uint8_t DO    = 253;
  static constexpr uint8_t WONT  = 252;
  static constexpr uint8_t WILL  = 251;
  static constexpr uint8_t SB    = 250;
  static constexpr uint8_t SE    = 240;

  // Options
  static constexpr uint8_t OPT_BINARY = 0;
  static constexpr uint8_t OPT_ECHO   = 1;
  static constexpr uint8_t OPT_SGA    = 3;

  int sendAll(int s, const void* data, size_t len);

  void doTelnetNegotiation(const uint8_t* buf, int len, std::string& out);

  void sendIAC(uint8_t cmd, uint8_t opt);

private:
  int         _sock      = -1;
  std::string _rx;
  std::string _lastError;
};
