#include "UartSnifferService.h"

#include <array>
#include <queue>
#include <string>

#include "Interfaces/IInput.h"
#include "Interfaces/ITerminalView.h"
#include "Interfaces/IUartPort.h"
#include "Interfaces/IUtilityService.h"

UartSnifferService::UartSnifferService(IUartPort& firstPort,
                                       HardwareSerial* firstSerial,
                                       IUartPort& secondPort,
                                       HardwareSerial* secondSerial)
    : firstPort(firstPort),
      firstSerial(firstSerial),
      secondPort(secondPort),
      secondSerial(secondSerial) {}

void UartSnifferService::configurePorts(unsigned long baud,
                                        uint32_t config,
                                        bool inverted,
                                        uint8_t rxPin1,
                                        uint8_t rxPin2) {
    const int8_t noTxPin = -1;

    firstPort.configure(baud, config, rxPin1, noTxPin, inverted, firstSerial, true);
    secondPort.configure(baud, config, rxPin2, noTxPin, inverted, secondSerial, true);

    firstPort.flush();
    secondPort.flush();
    while (firstPort.available()) { firstPort.read(); }
    while (secondPort.available()) { secondPort.read(); }
    firstPort.setRxFIFOFull(1);
    secondPort.setRxFIFOFull(1);
}

void UartSnifferService::sniffRaw(ITerminalView& terminalView,
                                  IInput& terminalInput,
                                  IUtilityService& utilityService,
                                  unsigned long baud,
                                  uint32_t config,
                                  bool inverted,
                                  uint8_t rxPin1,
                                  uint8_t rxPin2) {
    enum source {NONE, UART1, UART2};
    const char toHex[]={'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    source lastUart = NONE;
    bool uartChanged = true;
    uint8_t carDisplayed = 0;
    const uint8_t maxCarPerLine = 16;
    std::array<char, 81> lineBuffer;
    lineBuffer.fill(0x20); lineBuffer[80]=0; lineBuffer[48]='|';
    uint32_t lastUpdateMixed = utilityService.nowMs();
    constexpr uint32_t TIMEOUT_MIXED = 2000;

    struct rcvSer{
        source uart;
        char key;
    };

    std::queue<rcvSer> fifo;
    rcvSer rcv, snd;

    configurePorts(baud, config, inverted, rxPin1, rxPin2);

    while (true) {
        char key = terminalInput.readChar();
        if (key == '\r' || key == '\n') {
            terminalView.println("\n\r\n\rUART Sniff: Stopped by user.");
            break;
        }

        if (firstPort.available() > 0) {
            rcv.uart = UART1;
            rcv.key = firstPort.read();
            fifo.push(rcv);
        }

        if (secondPort.available() > 0) {
            rcv.uart = UART2;
            rcv.key = secondPort.read();
            fifo.push(rcv);
        }

        if (!fifo.empty()){
            snd = fifo.front();
            fifo.pop();

            if (snd.uart != lastUart){
                lastUart = snd.uart;
                uartChanged = true;
            }

            if (uartChanged && carDisplayed > 0){
                terminalView.print(lastUart == UART2 ? "\n\r[RX] " : "\n\r\t[TX] ");
                terminalView.print(lineBuffer.data());
                lineBuffer.fill(0x20); lineBuffer[80]=0; lineBuffer[48]='|';
                carDisplayed = 0;
                uartChanged = false;
                lastUpdateMixed = utilityService.nowMs();
            }

            lineBuffer[3 * carDisplayed] = toHex[(snd.key >> 4) & 0x0F];
            lineBuffer[3 * carDisplayed + 1] = toHex[snd.key & 0x0F];
            lineBuffer[50 + carDisplayed] = (snd.key >= ' ') ? snd.key : '.';
            carDisplayed++;
            uartChanged = false;
            if (carDisplayed >= maxCarPerLine){
                terminalView.print(lastUart == UART1 ? "\n\r[RX] " : "\n\r\t[TX] ");
                terminalView.print(lineBuffer.data());
                lineBuffer.fill(0x20); lineBuffer[80]=0; lineBuffer[48]='|';
                carDisplayed = 0;
                lastUpdateMixed = utilityService.nowMs();
            }
        }

        if ((carDisplayed > 0) && ((utilityService.nowMs() - lastUpdateMixed) > TIMEOUT_MIXED)){
            terminalView.print(lastUart == UART1 ? "\n\r[RX] " : "\n\r\t[TX] ");
            terminalView.print(lineBuffer.data());
            lineBuffer.fill(0x20); lineBuffer[80]=0; lineBuffer[48]='|';
            carDisplayed = 0;
            lastUpdateMixed = utilityService.nowMs();
        }

        utilityService.sleepMs(0);
    }

    firstPort.end();
    secondPort.end();
}

void UartSnifferService::sniffText(ITerminalView& terminalView,
                                   IInput& terminalInput,
                                   IUtilityService& utilityService,
                                   unsigned long baud,
                                   uint32_t config,
                                   bool inverted,
                                   uint8_t rxPin1,
                                   uint8_t rxPin2) {
    enum source {NONE, UART1, UART2};
    source lastUart = NONE;
    bool uartChanged = true;
    uint8_t carDisplayed = 0;
    const uint8_t maxCarPerLine = 80;

    struct rcvSer{
        source uart;
        char key;
    };

    std::queue<rcvSer> fifo;
    rcvSer rcv, snd;

    configurePorts(baud, config, inverted, rxPin1, rxPin2);

    while (true) {
        char key = terminalInput.readChar();
        if (key == '\r' || key == '\n') {
            terminalView.println("\n\r\n\rUART Sniff: Stopped by user.");
            break;
        }

        if (firstPort.available() > 0) {
            rcv.uart = UART1;
            rcv.key = firstPort.read();
            fifo.push(rcv);
        }

        if (secondPort.available() > 0) {
            rcv.uart = UART2;
            rcv.key = secondPort.read();
            fifo.push(rcv);
        }

        if (!fifo.empty()){
            snd = fifo.front();
            fifo.pop();

            if (snd.uart != lastUart){
                lastUart = snd.uart;
                uartChanged = true;
            }

            if (uartChanged || carDisplayed >= maxCarPerLine){
                terminalView.print(snd.uart == UART1 ? "\n\r[RX] " : "\n\r\t[TX] ");
                carDisplayed = 0;
                uartChanged = false;
            }
            if (snd.key >= ' '){
                terminalView.print(std::string(1, snd.key));
            } else {
                terminalView.print(" ");
            }
            carDisplayed++;
        }

        utilityService.sleepMs(0);
    }

    firstPort.end();
    secondPort.end();
}
