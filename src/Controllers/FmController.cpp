#include "FmController.h"
#include <sstream>
#include <iomanip>

/*
Constructor
*/
FmController::FmController(
    ITerminalView& terminalView,
    IInput& terminalInput,
    IDeviceView& deviceView,
    FmService& fmService,
    ArgTransformer& argTransformer,
    UserInputManager& userInputManager,
    HelpShell& helpShell,
    FmBroadcastShell& fmBroadcastShell
)
    : terminalView(terminalView),
      terminalInput(terminalInput),
      deviceView(deviceView),
      fmService(fmService),
      argTransformer(argTransformer),
      userInputManager(userInputManager),
      helpShell(helpShell),
      fmBroadcastShell(fmBroadcastShell)
{}

/*
Entry point for fm command
*/
void FmController::handleCommand(const TerminalCommand& cmd) {
    const auto& root = cmd.getRoot();

    if (root == "config") handleConfig();
    else if (root == "sweep") handleSweep();
    else if (root == "trace") handleTrace(cmd);
    else if (root == "waterfall") handleWaterfall();
    else if (root == "broadcast") handleBroadcast();
    else if (root == "reset") handleReset();
    else handleHelp();
}

/*
Ensure configured
*/
void FmController::ensureConfigured() {
    if (!configured) handleConfig();
    else {
        fmService.configure(state.getTwoWireRstPin(), state.getTwoWireIoPin(), state.getTwoWireClkPin(), state.getI2cFrequency());
    }
}

/*
Ensure present
*/
bool FmController::ensurePresent_() {
    if (!configured) return false;

    if (!fmService.begin()) {
        terminalView.println("FM: SI4713 not detected (I2C 0x63).");
        terminalView.println("Tip: check wiring, reset GPIO required\n");
        return false;
    }
    return true;
}

/*
Config
*/
void FmController::handleConfig() {

    terminalView.println("FM (SI4713) configuration.\n");

    const auto& forbidden = state.getProtectedPins();

    // I2C
    uint8_t sdaPin = (uint8_t)userInputManager.readValidatedPinNumber("SI4713 SDA GPIO", state.getTwoWireIoPin(), forbidden);
    uint8_t sclPin = (uint8_t)userInputManager.readValidatedPinNumber("SI4713 SCL GPIO", state.getTwoWireClkPin(), forbidden);

    // RST
    terminalView.println("\n [ℹ️  INFORMATIONS]");
    terminalView.println(" Reset GPIO is required, or you can");
    terminalView.println(" briefly connect SI4713 RST to GND");
    terminalView.println(" to activate the device.\n");
    int8_t resetPin = (int8_t)userInputManager.readValidatedPinNumber("SI4713 RESET GPIO", state.getTwoWireRstPin(), forbidden); // -1 = none

    // Freq
    uint32_t i2cFreqHz = userInputManager.readValidatedUint32("I2C frequency (Hz)", state.getI2cFrequency());

    state.setTwoWireClkPin(sclPin);
    state.setTwoWireIoPin(sdaPin);
    state.setTwoWireRstPin(resetPin);
    state.setI2cFrequency(i2cFreqHz);
    configured = true; // consider configured to avoid looping

    // Hard reset before configuring
    // if it stucks, it can't be detected
    // and then, can't be resetted by the lib
    fmService.reset(resetPin);

    if (!fmService.configure(resetPin, sdaPin, sclPin, i2cFreqHz)) {
        terminalView.println("\n❌ SI4713 configure failed.\n");
        return;
    }

    if (!fmService.begin()) {
        terminalView.println("\n❌ SI4713 not detected.\n");
        return;
    } else {
        terminalView.println("\n✅ SI4713 configured successfully.\n");
    }
}

/*
Sweep
*/
void FmController::handleSweep()
{
    ensureConfigured();
    if (!ensurePresent_()) return;

    int startMHz = userInputManager.readValidatedInt("Sweep start MHz", 87, 76, 108);
    int endMHz   = userInputManager.readValidatedInt("Sweep end MHz",   108, 76, 108);
    if (endMHz <= startMHz) endMHz = startMHz + 1;

    float step = userInputManager.readValidatedFloat("Sweep step MHz", 0.1, 0.1, 2);
    int step10kHz = (int)(step * 100);
    int samples   = userInputManager.readValidatedInt("Samples per frequency", 1, 1, 50);
    int thrNoise  = userInputManager.readValidatedInt("Print threshold (0=all)", 45, 0, 255);

    terminalView.println("\nFM Sweep: Analyzing frequencies... Press [ENTER] to stop.");

    uint16_t start10 = (uint16_t)(startMHz * 100);
    uint16_t end10   = (uint16_t)(endMHz * 100);

    int bins = (int)((end10 - start10) / step10kHz) + 1;
    if (bins < 2) bins = 2;

    int bestNoise = -1;
    uint16_t bestFreq10 = 0;
    int i = 0;

    while (true) {
        // Stop on ENTER (fast path)
        char c = terminalInput.readChar();
        if (c == '\n' || c == '\r') break;

        // New sweep header once per loop
        if (i == 0) {
            if (bestFreq10 != 0 && bestNoise >= 0) {
                std::stringstream ss;
                ss << "\nPEAK: " << std::fixed << std::setprecision(2)
                   << (bestFreq10 / 100.0f) << " MHz  noise=" << bestNoise << "\n";
                terminalView.println(ss.str());
            } else {
                terminalView.println("\nPEAK: --\n");
            }

            bestNoise = -1;
            bestFreq10 = 0;
        }

        uint16_t f10 = (uint16_t)(start10 + (uint16_t)(i * step10kHz));
        if (f10 > end10) f10 = end10;

        // Measure peak noise (no delay)
        uint8_t peakNoise = 0;
        bool abort = false;

        for (int s = 0; s < samples; ++s) {
            char cc = terminalInput.readChar();
            if (cc == '\n' || cc == '\r') { abort = true; break; }

            uint8_t n = 0;
            if (!fmService.measureAt(f10, n)) {
                terminalView.println("FM Sweep: measure failed (I2C).");
                abort = true;
                break;
            }
            if (n > peakNoise) peakNoise = n;
        }

        if (abort) break;

        // Track best peak for this sweep
        if ((int)peakNoise > bestNoise) {
            bestNoise = (int)peakNoise;
            bestFreq10 = f10;
        }

        // Print line
        if (thrNoise == 0 || (int)peakNoise >= thrNoise) {

            std::stringstream ss;

            // Format frequency as fixed width "XXX.XX"
            ss << "  "
            << std::setw(3) << std::setfill(' ') << (f10 / 100)  // MHz padded left
            << "."
            << std::setw(2) << std::setfill('0') << (f10 % 100)  // decimals padded with 0
            << " MHz  ";

            // Align noise column too
            ss << "noise="
            << std::setw(3) << std::setfill(' ') << (int)peakNoise;

            // Show peak info aligned
            if (bestFreq10 != 0) {
                ss << "  (peak="
                << std::setw(3) << std::setfill(' ') << bestNoise
                << ")";
            }

            terminalView.println(ss.str());
        }

        // Next bin
        i++;
        if (i >= bins) i = 0;
    }

    terminalView.println("\nFM Sweep: Stopped by user.\n");
}

/*
Trace
*/
void FmController::handleTrace(const TerminalCommand& cmd)
{
    ensureConfigured();
    if (!ensurePresent_()) return;

    uint16_t f10 = 0;
    float fMHz = 0.0f;
    if (cmd.getSubcommand().empty()) {
        fMHz = userInputManager.readValidatedFloat("Trace frequency MHz", 100.0f, 76.0f, 108.0f);
        f10 = (uint16_t)(fMHz * 100.0f + 0.5f);
    } else if (argTransformer.isValidFloat(cmd.getSubcommand())) {
        fMHz = std::stof(cmd.getSubcommand());
        f10 = (uint16_t)(fMHz * 100.0f + 0.5f);
    } else {
        terminalView.println("Usage: trace [freqMhz]");
        return;
    }

    // intensification
    int gain = 16;

    terminalView.println("\nFM Trace: Displaying on the ESP32 screen... Press [ENTER] to stop.");

    deviceView.clear();
    deviceView.topBar(argTransformer.toFixed2(fMHz) + " MHz", false, false);

    std::vector<uint8_t> buffer;
    buffer.reserve(240);

    unsigned long lastPoll = millis();

    // Baseline EMA 
    int32_t ema = -1;
    const int EMA_SHIFT = 4; // more responsive EMA
    uint8_t prevOut = 128;
    bool hasPrev = false;
    const int it = 40;

    while (true) {
        // Cancel
        if (millis() - lastPoll >= 10) {
            lastPoll = millis();
            const char c = terminalInput.readChar();
            if (c == '\n' || c == '\r') {
                terminalView.println("FM Trace: Stopped by user.\n");
                return;
            }
        }

        uint8_t n = 0;
        if (!fmService.measureAt(f10, n)) {
            terminalView.println("FM Trace: measure failed (I2C).");
            return;
        }

        // Init EMA
        if (ema < 0) ema = ((int32_t)n) << 8;

        // EM
        int32_t x = ((int32_t)n) << 8;
        ema += (x - ema) >> EMA_SHIFT;
        int base = (int)(ema >> 8);

        // Delta amplification
        int d = (int)n - base;

        // normalize and ramp
        int y = 128 + d * gain;
        if (!hasPrev) { prevOut = y; hasPrev = true; }
        int a = (int)prevOut;
        int b = (int)y;

        // speed up display refresh
        for (int k = 1; k <= it; ++k) {
            // y = a + (b-a)*k/N
            int yy = a + ((b - a) * k) / it;
            buffer.push_back((uint8_t)yy);
        }

        if (buffer.size() >= 240) {
            buffer.resize(240);
            deviceView.drawAnalogicTrace(0, buffer, 1);
            buffer.clear();
        }
    }
}

/*
Waterfall
*/
void FmController::handleWaterfall()
{
    ensureConfigured();
    if (!ensurePresent_()) return;

    // Range MHz
    int startMHz = userInputManager.readValidatedInt("Waterfall start MHz", 87, 76, 108);
    int endMHz   = userInputManager.readValidatedInt("Waterfall end MHz",   108, 76, 108);
    if (endMHz <= startMHz) endMHz = startMHz + 1;

    // Step in 10kHz units
    float step = userInputManager.readValidatedFloat("Waterfall step MHz", 0.1, 0.1, 2);
    int step10kHz = (int)(step * 100);

    // Samples per frequency
    int samples = userInputManager.readValidatedInt("Samples per frequency", 1, 1, 50);

    terminalView.println("\nFM Waterfall: Displaying on the ESP32 screen... Press [ENTER] to stop.");

    uint16_t start10 = (uint16_t)(startMHz * 100);
    uint16_t end10   = (uint16_t)(endMHz * 100);

    int bins = (int)((end10 - start10) / step10kHz) + 1;
    if (bins < 2) bins = 2;

    float fStart = start10 / 100.0f;
    float fEnd   = end10 / 100.0f;

    // Noise mapping 
    const int noiseMin = 10;
    const int noiseMax = 80;

    int bestNoise = -1;
    uint16_t bestFreq10 = 0;
    std::string title;
    int i = 0;

    while (true) {
        // Stop on ENTER
        char c = terminalInput.readChar();
        if (c == '\n' || c == '\r') break;

        // Display peak once per sweep
        if (i == 0) {
            if (bestFreq10 != 0 && bestNoise >= 0) {
                std::stringstream ss;
                ss << "PEAK: " << std::fixed << std::setprecision(2)
                   << (bestFreq10 / 100.0f) << "MHz noise=" << bestNoise;
                title = ss.str();
            } else {
                title = "PEAK: --";
            }
            bestNoise = -1;
            bestFreq10 = 0;
        }

        // Frequency
        uint16_t f10 = (uint16_t)(start10 + (uint16_t)(i * step10kHz));
        if (f10 > end10) f10 = end10;

        // Measure peak noise for this frequency
        uint8_t peakNoise = 0;
        bool abort = false;
        for (int s = 0; s < samples; ++s) {
            char cc = terminalInput.readChar();
            if (cc == '\n' || cc == '\r') { abort = true; break; }

            uint8_t n = 0;
            if (!fmService.measureAt(f10, n)) {
                // I2C error, stop waterfall
                terminalView.println("FM Waterfall: measure failed (I2C).");
                abort = true;
                break;
            }

            if (n > peakNoise) peakNoise = n;
        }

        if (abort) break;

        // Track best peak across the sweep
        if ((int)peakNoise > bestNoise) {
            bestNoise = (int)peakNoise;
            bestFreq10 = f10;
        }

        // Map to level %
        int v = (int)peakNoise;
        if (v < noiseMin) v = noiseMin;
        if (v > noiseMax) v = noiseMax;

        int level = (int)((int64_t)(v - noiseMin) * 100 / (noiseMax - noiseMin));
        if (level < 1) level = 1;

        deviceView.drawWaterfall(
            title,
            fStart,
            fEnd,
            "MHz",
            i,
            bins,
            level
        );

        // Next
        i++;
        if (i >= bins) i = 0;
    }

    terminalView.println("FM Waterfall: Stopped by user.\n");
}

/*
Broadcast
*/
void FmController::handleBroadcast() {
    if (!ensurePresent_()) return;
    fmBroadcastShell.run();
}

/*
Reset
*/
void FmController::handleReset() {
    terminalView.println("FM: Resetting SI4713 via reset GPIO...");
    fmService.reset(state.getTwoWireRstPin());
}

/*
Help
*/
void FmController::handleHelp() {
    terminalView.println("\nUnknown command. Available FM commands:");
    helpShell.run(state.getCurrentMode(), false);
}
