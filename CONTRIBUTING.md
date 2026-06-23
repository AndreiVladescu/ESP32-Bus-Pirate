
# How to Contribute
Contributing to this firmware is **simple**. Below we describe a **straightforward method to add a new command** to the project.

As a contributor, you usually only need to work on the **Controller** and **Service** related to the protocol where you want to add your command.

If you are not familiar with PlatformIO, you can follow the build instructions here:  
https://github.com/geo-tp/ESP32-Bit-Pirate/wiki/99-Build


## Basic Steps

### 1. Create a `handleXXX` Method in the Appropriate Controller

Each terminal command is handled in a *Controller* (e.g., `UartController`, `I2cController`, `LedController`, etc.).

Example: `src/Controllers/UartController`
```cpp
void UartController::handleRead() {
    terminalView.println("UART Read: Streaming until [ENTER] is pressed...");
    uartService.flush();

    while (true) {
        // Stop if ENTER is pressed
        char key = terminalInput.readChar(); // read a char, non blocking
        if (key == '\r' || key == '\n') {
            terminalView.println("\nUART Read: Stopped by user.");
            break;
        }

        // Print UART data as it comes
        while (uartService.available() > 0) {
            char c = uartService.read();
            terminalView.print(std::string(1, c)); // print to the terminal
        }
    }
}
```

---

### 2. Use `terminalView` and `terminalInput` for I/O

- `terminalView` handles output (print, println) in the web or serial terminal.
```cpp
class ITerminalView {
public:
    virtual ~ITerminalView() = default;
    
    // Initialize the terminal
    virtual void initialize() = 0;

    // Show welcome message with logo and infos
    virtual void welcome(TerminalTypeEnum& terminalType, std::string& terminalInfos) = 0;

    // Print to the terminal
    virtual void print(const std::string& text) = 0;
    virtual void println(const std::string& text) = 0;
    virtual void printPrompt(const std::string& mode = "HIZ") = 0;

    // Wait press
    virtual void waitPress() = 0;

    // Clear the terminal
    virtual void clear() = 0;
};

```

- `terminalInput` handles input (readChar for keypresses, etc.) in the web or serial terminal.
```cpp
class IInput {
public:
    virtual ~IInput() = default;

    // Blocking read
    virtual char handler() = 0;

    // Non blocking read
    virtual char readChar() = 0;

    // Wait an input
    virtual void waitPress() = 0;
    
};
```

---

### 3. Use Helpers to validate and parse arguments

Your `handleXXX` method can optionally take a `const TerminalCommand& cmd` parameter, which contains the user's input split into `root`, `subcommand`, and `args`.

For example, the UART command:

```cpp
void UartController::handleSpam(const TerminalCommand& cmd); // Usage: spam <text> <ms>
```

Will be interpreted as:

- `cmd.getRoot() -> "spam"`
- `cmd.getSubCommand() -> "Hello"`
- `cmd.getArgs() -> "10"`

You can then use `ArgTransformer` to parse and validate these arguments as needed.

- `ArgTransformer` to validate and convert strings into `uint8_t`, `uint32_t`, hex, etc.
- `UserInputManager` for interactive prompts and validation (yes/no, pin number, validated strings, hex list, etc.).

Example:
```cpp

// Split args into a list of string
std::vector<std::string> args = argTransformer.splitArgs(cmd.getArgs());

// Validate string arg
bool valid = argTransformer.isValidNumber(args[0])

// Transform string arg into an unsigned int
uint8_t pin = argTransformer.toUint8(args[0]);

// Ask and validate a char choice
char parityChar = userInputManager.readCharChoice("Parity (N/E/O)", defaultParity, {'N', 'E', 'O'});

// Ask yes or no
bool inverted = userInputManager.readYesNo("Inverted?", state.isUartInverted());

```

---

### 4. Use `GlobalState` to Share State Between Controllers

The `GlobalState` object provides a centralized way to share data across controllers.

You can use it to:
- Store and access **pin mappings** for each protocol (e.g., UART RX/TX pins, I2C SCL/SDA).
- Share **global configuration or flags** (e.g., current terminal mode, webui IP, etc).

Example:
```cpp
int rxPin = state.getUartRxPin();
state.setUartRxPin(rxPin);
```

---

### 5. Delegate Low-Level Logic to Services

The **Controller** is for high-level logic and user interaction.
The **Service** handles protocol-level actions.

Example:
```cpp
char UartService::read() {
    return Serial1.read();
}

void UartService::write(char c) {
    Serial1.write(c);
}

```

---

### 📎 Summary

These components are already integrated into each controller, you can simply use them directly in your new handleXXX method.


| Component         | Responsibility                  |
|------------------|----------------------------------|
| service (uartService, i2cService, etc)          | Low-level protocol logic        |
| terminalView      | Output to terminal (web and serial)             |
| terminalInput     | Input from user (web and serial)               |
| argTransformer    | String arguments to typed value            |
| userInputManager  | Ask for a certain input type and validate it  |
| state  | Application state shared between controllers |

Example:

```cpp
void FictiveController::handleXXX(const TerminalCommand& cmd) {
    terminalView.println("[FICTIVE] Command execution started.");

    // Read arguments from the TerminalCommand
    std::string rawValue = cmd.getSubCommand();
    if (rawValue.empty()) {
        terminalView.println("[FICTIVE] No value provided. Please enter a number");
        return;
    }

    // Validate string using ArgTransformer
    if (!argTransformer.isValidNumber(rawValue)) {
        terminalView.println("[FICTIVE] Invalid number format.");
        return;
    }
   
    // Convert string arg into int using ArgTransformer
    uint8_t value = argTransformer.toUint8(rawValue);
   
    // Ask user confirmation using UserInputManager
    bool confirmation = userInputManager.readYesNo("Are you sure?", false);
    if (!confirmation) {
        terminalView.println("[FICTIVE] User is not sure. Stopped.");
        return;
    }

    // Use service logic (e.g., simulate sending the value somewhere)
    bool result = fictiveService.sendValue(value);
    if (result) {
        terminalView.println("[FICTIVE] Value sent successfully.");
    } else {
        terminalView.println("[FICTIVE] Failed to send value.");
    }

    // Access shared state (e.g., configured pin for this protocol)
    int pin = state.getFictiveDataPin();
    terminalView.println("[FICTIVE] Used data pin: " + std::to_string(pin));
}

```
