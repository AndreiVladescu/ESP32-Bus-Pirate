#include <string>
#include <sstream>
#include <iomanip>

#include "Services/OneWireService.h"
#include "Interfaces/IInput.h"
#include "Interfaces/ITerminalView.h"
#include "Models/TerminalCommand.h"
#include "States/GlobalState.h"
#include "Transformers/ArgTransformer.h"
#include "Managers/UserInputManager.h"

class OneWireController {
public:
    // Constructor
    OneWireController(ITerminalView& terminalView, IInput& terminalInput, 
                      OneWireService& service, ArgTransformer& argTransformer,
                      UserInputManager& userInputManager);

    // Entry point for handle command
    void handleCommand(const TerminalCommand& cmd);

    // Entry point for handle parsed bytecode instructions
    void handleInstruction(std::vector<ByteCode>& bytecodes);

    // Ensure initialized/configured
    void ensureConfigured();

private:
    ITerminalView& terminalView;
    IInput& terminalInput;
    OneWireService& oneWireService;
    ArgTransformer& argTransformer;
    UserInputManager& userInputManager;
    GlobalState& state = GlobalState::getInstance();
    bool configured = false;

  // Configure 1-Wire bus parameters
    void handleConfig();

    // Send reset pulse and check device presence
    void handlePing();

    // Scan the bus for all devices
    void handleScan();

    // Listen to bus traffic passively
    void handleSniff();

    // Read a byte or data block from the bus
    void handleRead();

    // Print available 1-Wire commands
    void handleHelp();

    // Read and display a device's ROM ID
    bool handleIdRead();

    // Read device scratchpad memory
    bool handleScratchpadRead();

    // Write raw data to the bus
    void handleWrite(const TerminalCommand& command);

    // Write a ROM ID to a device (e.g. iButton emulator)
    void handleIdWrite(std::vector<uint8_t> idBytes);

    // Copy 1wire data
    void handleCopy(const TerminalCommand& command);
    
    // Read a source ibutton to copy its ID on another one
    void handleIdCopy();

    // Write to scratchpad memory
    void handleScratchpadWrite(std::vector<uint8_t> scratchpadBytes);

    // Read temperature sensor
    void handleTemperature();
};
