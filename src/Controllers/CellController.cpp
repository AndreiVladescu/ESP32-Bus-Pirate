#include "CellController.h"

/*
Constructor
*/
CellController::CellController(ITerminalView& view,
                               IInput& terminalInput,
                               CellService& service,
                               ArgTransformer& argTransformer,
                               AtTransformer& atTransformer,
                               UserInputManager& userInputManager,
                               HelpShell& helpShell,
                               CellCallShell& cellCallShell,
                               CellSmsShell& cellSmsShell)
    : terminalView(view),
      terminalInput(terminalInput),
      cellService(service),
      argTransformer(argTransformer),
      atTransformer(atTransformer),
      userInputManager(userInputManager),
      helpShell(helpShell),
      cellCallShell(cellCallShell),
      cellSmsShell(cellSmsShell)
{
}

/*
Entry point for cell command
*/
void CellController::handleCommand(const TerminalCommand& command)
{
    const std::string& root = command.getRoot();

    if (root == "config")         handleConfig();
    else if (root == "modem")     handleModem();  
    else if (root == "network")   handleNetwork();
    else if (root == "operator")  handleOperator();
    else if (root == "sim")       handleSim();
    else if (root == "unlock")    handleUnlock(command); 
    else if (root == "phonebook") handlePhoneBook();
    else if (root == "setmode")   handleSetMode();
    else if (root == "sms")       handleSms(command); 
    else if (root == "ussd")      handleUssd(command); 
    else if (root == "call")      handleCall(command); 
    else handleHelp();
}

/*
Ensure configured
*/
void CellController::ensureConfigured()
{
    if (!configured) {
        handleConfig();
        return;
    }

    cellService.init(state.getUartRxPin(), state.getUartTxPin(), state.getUartBaudRate());
}

/*
Config
*/
void CellController::handleConfig()
{
    terminalView.println("Cell configuration");

    const auto& forbidden = state.getProtectedPins();

    uint8_t rx = userInputManager.readValidatedPinNumber("MODEM RX GPIO", state.getUartRxPin(), forbidden);
    state.setUartRxPin(rx);

    uint8_t tx = userInputManager.readValidatedPinNumber("MODEM TX GPIO", state.getUartTxPin(), forbidden);
    state.setUartTxPin(tx);

    const uint32_t defaultModemBaud = 115200; // common default baudrate for modems
    uint32_t baud = userInputManager.readValidatedUint32("Baudrate", defaultModemBaud);
    state.setUartBaudRate(baud);

    cellService.init(rx, tx, baud);
    configured = true; // consider configured to not loop back to config 

    terminalView.println("\nDetecting modem...");
    if (!cellService.detect()) {
        terminalView.println("❌ No modem detected.");
        terminalView.println("It may take up 30 sec to start.");
        terminalView.println("Try to swap RX and TX GPIOs.\n");
        return;
    }

    terminalView.println("✅ Modem detected");
    terminalView.println(atTransformer.formatModuleInfo(cellService.getModuleInfo()));
    terminalView.println("");
}

/*
Modem
*/
void CellController::handleModem()
{
    terminalView.println("\n[MODEM INFO]");
    terminalView.println("  " + atTransformer.formatManufacturer(cellService.getManufacturer()));
    terminalView.println("  Model: " + atTransformer.clean(cellService.getModel()));
    terminalView.println("  " + atTransformer.clean(cellService.getRevision()));
    terminalView.println("  " + atTransformer.formatImei(cellService.getImei()));
    terminalView.println("  " + atTransformer.formatFunctionality(cellService.getFunctionality())); 
    terminalView.println("  Clock: " + atTransformer.formatClock(cellService.getClock()));
    terminalView.println("\n  Use 'setmode' command to change mode.");
    terminalView.println("");
}

/*
SIM
*/
void CellController::handleSim()
{
    terminalView.println("\n[SIM CARD]");

    terminalView.println("  " + atTransformer.formatSimState(cellService.getSimState()));
    terminalView.println("  " + atTransformer.formatSpn(cellService.getServiceProviderName()));
    
    terminalView.println("  " + atTransformer.formatIccid(cellService.getIccid()));
    terminalView.println("  " + atTransformer.formatImsi(cellService.getImsi()));
    terminalView.println("  " + atTransformer.formatMsisdn(cellService.getMsisdn()));
    
    terminalView.println("  " + atTransformer.formatPinLock(cellService.getPinLockStatus())); 
    terminalView.println("  " + atTransformer.formatSimRetries(cellService.getSimRetries()));

    terminalView.println("  " + atTransformer.formatPhonebookStorage(cellService.getPhonebookStorage()));
    terminalView.println("  " + atTransformer.formatSmsStorage(cellService.getSmsStorage()));

    if (!cellService.isSimReady()) {
        terminalView.println("\n  Use 'unlock' command to enter SIM PIN.");
    }

    terminalView.println("");
}

/*
Network
*/
void CellController::handleNetwork()
{
    terminalView.println("\n[NETWORK STATUS]");
    terminalView.println("  " + atTransformer.formatSignal(cellService.getSignal()));
    terminalView.println("  " + atTransformer.formatOperator(cellService.getOperator()));
    terminalView.println("  " + atTransformer.formatRegistrationCS(cellService.getRegistrationCS()));
    terminalView.println("  " + atTransformer.formatRegistrationPS(cellService.getRegistrationPS()));
    terminalView.println("  " + atTransformer.formatAttach(cellService.getAttach()));
    terminalView.println("");

    terminalView.println("[PDP]");
    terminalView.println("  " + atTransformer.formatPdpContexts(cellService.queryPdpContexts()));
    terminalView.println("  " + atTransformer.formatPdpActive(cellService.queryPdpActive()));
    terminalView.println("");
}

/*
Operator
*/
void CellController::handleOperator()
{
    if (!configured) return;

    terminalView.println("\nCell Operator: Scanning... (can take up to 1 minute)\n");

    std::string scan = cellService.scanOperators(); // 60 seconds timeout

    terminalView.println(atTransformer.formatScanOperators(scan));

    terminalView.println("Current " + atTransformer.formatOperator(cellService.getOperator()));
    bool autoMode = userInputManager.readYesNo("Reset modem operator to auto mode?", false);

    if (autoMode) {
        bool ok = cellService.setOperatorAuto();
        terminalView.println(ok ? "✅ Set operator auto: OK\n" : "❌ Set operator auto: ERROR\n");
    } else {
        terminalView.println(""); // align
    }
}

/*
Unlock SIM (enter PIN or PUK if required)
*/
void CellController::handleUnlock(const TerminalCommand& command)
{
    // PUK
    if (cellService.isSimPukRequired()) {
        terminalView.println("\n [⚠️  Warning]");
        terminalView.println(" This SIM is locked and can't be unlocked with PIN.");
        terminalView.println(" A valid PUK code is now required to unblock it.");
        terminalView.println(" You must also choose a new PIN after entering the PUK.\n");

        auto proceed = userInputManager.readYesNo("Continue with PUK unlock?", false);
        if (!proceed) {
            terminalView.println("SIM unlock cancelled.\n");
            return;
        }

        std::string puk = userInputManager.readValidatedNumericCode("Enter PUK", "", 8, 8);
        std::string newPin = userInputManager.readValidatedNumericCode("Enter new PIN", "", 4, 8);

        if (puk.empty() || newPin.empty()) {
            terminalView.println("PUK and new PIN are required for unlock. Canceled.\n");
            return;
        }

        auto confirm = userInputManager.readYesNo("Unlock SIM with entered PUK and new PIN?", false);
        if (!confirm) {
            terminalView.println("SIM unlock cancelled.\n");
            return;
        }

        bool ok = cellService.enterPuk(puk, newPin);
        terminalView.println(ok ? "PUK accepted (OK)." : "PUK failed (ERROR).");
        delay(100);
        terminalView.println(atTransformer.formatSimState(cellService.getSimState()) + "\n");
        return;
    }

    // Already ready
    if (cellService.isSimReady()) {
        terminalView.println("\n [⚠️  Warning]");
        terminalView.println(" SIM is already ready. No need to unlock.\n");
        auto confirm = userInputManager.readYesNo("Do you want to re-enter PIN anyway?", false);
        if (!confirm) {
            terminalView.println("SIM unlock cancelled.\n");
            return; 
        }
    }

    // Enter PIN
    std::string pin = command.getSubcommand();
    if (pin.empty()) pin = command.getArgs();

    if (pin.empty()) {
        pin = userInputManager.readValidatedNumericCode("Enter SIM PIN", "", 4, 8);
    }

    if (!argTransformer.isValidNumericCode(pin, 4, 8)) {
        terminalView.println("Invalid PIN format. Must be 4-8 digits.");
        return;
    }

    auto confirm = userInputManager.readYesNo("Unlock with entered PIN?", false);
    if (!confirm) {
        terminalView.println("SIM unlock cancelled.\n");
        return;
    }

    bool ok = cellService.enterPin(pin);
    terminalView.println(ok ? "PIN accepted (OK)." : "PIN failed (ERROR).");
    delay(200);
    terminalView.println(atTransformer.formatSimState(cellService.getSimState()) + "\n");
}

/*
Phonebook
*/
void CellController::handlePhoneBook()
{
    if (!configured) return;

    terminalView.println("\n[PHONEBOOK]");
    terminalView.println("  " + atTransformer.formatPhonebookStorage(cellService.getPhonebookStorage()));
    terminalView.println("  " + atTransformer.formatPhonebookCaps(cellService.getPhonebookCaps()));
    terminalView.println("");
    terminalView.println("[ENTRIES]");

    // Dump all entries 
    std::string resp = cellService.phonebookReadRange(1, 250);

    terminalView.println(atTransformer.formatPhonebookEntries(resp));
    terminalView.println("");
}

/*
SMS
*/
void CellController::handleSms(const TerminalCommand& command)
{
    if (cellService.isSimReady() == false) {
        terminalView.println("CELL: SIM not ready. Use 'unlock' command.");
        return;
    }

    cellSmsShell.run();
}

/*
USSD
*/
void CellController::handleUssd(const TerminalCommand& command)
{
    if (cellService.isSimReady() == false) {
        terminalView.println("CELL: SIM not ready. Use 'unlock' command.");
        return;
    }

    // ussd <code> [dcs]
    std::string code = command.getSubcommand();
    auto args = argTransformer.splitArgs(command.getArgs());

    // Interactive mode if nothing provided
    if (code.empty()) {
        terminalView.println("\n[USSD REQUEST]");

        code = userInputManager.readString("Enter USSD code", "*123#");

        const std::vector<std::string> dcsChoices = {
            "15 (Default GSM 7-bit)",
            "72 (UCS2 / Unicode)",
            "Exit"
        };

        int idx = userInputManager.readValidatedChoiceIndex("Select DCS encoding", dcsChoices, 0);
        if (idx == 2 || idx < 0) {
            terminalView.println("Cancelled.");
            return;
        }

        uint8_t dcs = (idx == 1) ? 72 : 15;

        terminalView.println("Sending USSD: " + code);
        bool ok = cellService.ussdRequest(code, dcs);
        terminalView.println(ok ? "USSD request: OK (response may arrive later)"
                               : "USSD request: ERROR");
        return;
    }

    bool valid = !code.empty() &&
                std::all_of(code.begin(), code.end(), [](char c) {
                    return (c >= '0' && c <= '9') || c == '*' || c == '#' || c == '+';
                });

    if (!valid) {
        terminalView.println("Invalid USSD code. Allowed: digits, '*', '#', '+'.");
        return;
    }

    // Provided args
    uint8_t dcs = 15;
    if (args.size() >= 2 && argTransformer.isValidNumber(args[1])) {
        dcs = (uint8_t)argTransformer.parseHexOrDec(args[1]);
    }

    bool ok = cellService.ussdRequest(code, dcs);
    terminalView.println(ok ? "USSD request: OK (response may arrive later)"
                           : "USSD request: ERROR");
}

/*
Call
*/
void CellController::handleCall(const TerminalCommand& command)
{
    if (cellService.isSimReady() == false) {
        terminalView.println("CELL: SIM not ready. Use 'unlock' command.");
        return;
    }

    cellCallShell.run();
}

/*
Set modem mode
*/
void CellController::handleSetMode()
{
    std::vector<std::string> choices = {
        "Full     (CFUN=1)   - normal mode",
        "Airplane (CFUN=4)   - RF disabled",
        "Exit"
    };

    int defaultIndex = (int)choices.size() - 1; // Exit selected by default

    int idx = userInputManager.readValidatedChoiceIndex("Select modem mode", choices, defaultIndex);
    if (idx == defaultIndex) {
        terminalView.println("Exit setmode....\n");
        return;
    }

    bool ok = false;
    
    terminalView.println("");
    switch (idx) {
        case 0: // CFUN=1
            ok = cellService.setFunctionality(1);
            terminalView.println(ok ? "CFUN=1: OK" : "CFUN=1: ERROR");
            break;

        case 1: // CFUN=4
            ok = cellService.setFunctionality(4);
            terminalView.println(ok ? "CFUN=4: OK" : "CFUN=4: ERROR");
            break;

        default:
            terminalView.println("Invalid choice.");
            return;
    }

    terminalView.println("");
    terminalView.println(atTransformer.formatRegistrationCS(cellService.getRegistrationCS()));
    terminalView.println(atTransformer.formatRegistrationPS(cellService.getRegistrationPS()));
}

/*
Help
*/
void CellController::handleHelp()
{
    helpShell.run(state.getCurrentMode(), false);
}
