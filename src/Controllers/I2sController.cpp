#include "I2sController.h"

/*
Constructor
*/
I2sController::I2sController(ITerminalView& terminalView, IInput& terminalInput,
                             I2sService& i2sService, ArgTransformer& argTransformer,
                             UserInputManager& userInputManager, HelpShell& helpShell)
    : terminalView(terminalView), terminalInput(terminalInput),
      i2sService(i2sService), argTransformer(argTransformer),
      userInputManager(userInputManager), helpShell(helpShell) {}


/*
Entry point for I2S command
*/
void I2sController::handleCommand(const TerminalCommand& cmd) {
    if      (cmd.getRoot() == "config")   handleConfig();
    else if (cmd.getRoot() == "play")     handlePlay(cmd);
    else if (cmd.getRoot() == "record")   handleRecord(cmd);
    else if (cmd.getRoot() == "test")     handleTest(cmd);
    else if (cmd.getRoot() == "reset")    handleReset();
    else handleHelp();
}

/*
Play
*/
void I2sController::handlePlay(const TerminalCommand& cmd) {
    switchOutputInput(true);
    auto args = argTransformer.splitArgs(cmd.getArgs());


    if (!argTransformer.isValidNumber(cmd.getSubcommand())) {
        terminalView.println("Usage: play <frequency> [durationMs]");
        return;
    }

    uint16_t freq = argTransformer.parseHexOrDec32(cmd.getSubcommand());

    // Infinite until ENTER press
    if (args.empty()) {
        terminalView.println("\nI2S Play: Tone @ " + std::to_string(freq) + " Hz (Press [ENTER] to stop)...\n");

        i2sService.playToneInterruptible(state.getI2sSampleRate(), freq, 0xFFFF, [&]() -> bool {
            char ch = terminalInput.readChar();
            return ch == '\n' || ch == '\r';
        });
    
    // Duration or until ENTER press
    } else if (args.size() == 1 && argTransformer.isValidNumber(args[0])) {
        uint16_t duration = argTransformer.parseHexOrDec32(args[0]);

        terminalView.println("\nI2S Play: Tone @ " + std::to_string(freq) + " Hz for " + std::to_string(duration) + " ms (or press [ENTER] to stop early)...\n");

        i2sService.playToneInterruptible(state.getI2sSampleRate(), freq, duration, [&]() -> bool {
            char ch = terminalInput.readChar();
            return ch == '\n' || ch == '\r';
        });

    } else {
        terminalView.println("Usage: play <frequency> [durationMs]");
        return;
    }

    terminalView.println("I2S Play: Done.");
}

/*
Record
*/
void I2sController::handleRecord(const TerminalCommand& cmd) {
    switchOutputInput(false);
    terminalView.println("I2S Record: In progress... Press [Enter] to stop.\n");

    constexpr size_t batchSize = 2048;
    constexpr size_t groupCount = 16;
    std::vector<int16_t> buffer(batchSize);

    int16_t dynamicMax = 5000; // initial value

    while (true) {
        size_t samplesRead = i2sService.recordSamples(buffer.data(), batchSize);
        size_t samplesPerGroup = samplesRead / groupCount;

        // Find peak on the batch
        int16_t batchPeak = 0;
        for (size_t i = 0; i < samplesRead; ++i) {
            int16_t val = abs(buffer[i]);
            if (val > batchPeak) batchPeak = val;
        }

        // Progressive update
        if (batchPeak > dynamicMax) dynamicMax = batchPeak;
        else dynamicMax = (dynamicMax * 9 + batchPeak) / 10;

        std::string line;
        for (size_t g = 0; g < groupCount; ++g) {
            int16_t peak = 0;
            for (size_t i = 0; i < samplesPerGroup; ++i) {
                size_t idx = g * samplesPerGroup + i;
                int16_t val = abs(buffer[idx]);
                if (val > peak) peak = val;
            }

            int level = (peak * 100) / (dynamicMax == 0 ? 1 : dynamicMax);
            if (level > 100) level = 100;
            if (level < 0) level = 0;

            char buf[6];
            sprintf(buf, "%03d ", level);
            line += buf;
        }

        terminalView.println(line);

        char ch = terminalInput.readChar();
        if (ch == '\n' || ch == '\r') break;
    }

    terminalView.println("\nI2S Record: Stopped by user.\n");
}

/*
Test
*/
void I2sController::handleTest(const TerminalCommand& cmd) {
    std::string mode = cmd.getSubcommand();

    if (mode.empty()) {
        terminalView.println("Usage: test <speaker|mic>");
        return;
    }

    if (mode[0] == 's') {
        handleTestSpeaker();
    }
    else if (mode[0] == 'm') {
        handleTestMic();
    }
    else {
        terminalView.println("Usage: test <speaker|mic>");
    }
}

/*
Test Speaker
*/
void I2sController::handleTestSpeaker() {
    switchOutputInput(true);
    terminalView.println("I2S Speaker Test: Running ... Press [ENTER] to stop\n");

    bool stop = false;

    // Helper to wait with check for stop on enter
    auto waitOrStop = [&](uint32_t ms) {
        for (uint32_t i = 0; i < ms; ++i) {
            char ch = terminalInput.readChar();
            if (ch == '\n' || ch == '\r') {
                stop = true;
                return;
            }
            delay(1);
        }
    };

    auto rate = state.getI2sSampleRate();

    terminalView.println("Using GPIOs:");
    terminalView.println("  BCLK : " + std::to_string(state.getI2sBclkPin()));
    terminalView.println("  LRCK : " + std::to_string(state.getI2sLrckPin()));
    terminalView.println("  DATA : " + std::to_string(state.getI2sDataPin()));
    terminalView.println("");

    // Melody
    terminalView.println("  Melody...");
    const uint16_t melody[] = {262, 294, 330, 349, 392, 440, 494, 523};
    for (uint16_t f : melody) {
        if (stop) break;
        i2sService.playTone(rate, f, 200);
        waitOrStop(50);
    }

    if (!stop) {
        waitOrStop(1000);
    }

    // Sweep
    if (!stop) {
        terminalView.println("  Frequency Sweep...");
        for (uint16_t f = 100; f <= 3000 && !stop; f += 300) {
            i2sService.playTone(rate, f, 100);
        }
        waitOrStop(800);
    }

    // Low
    if (!stop) {
        terminalView.println("  Low Frequency...");
        for (uint16_t f : {50,100,150,200,250,300,350,400,450,500}) {
            if (stop) break;
            i2sService.playTone(rate, f, 400);
            waitOrStop(100);
        }
        waitOrStop(800);
    }

    // High
    if (!stop) {
        terminalView.println("  High Frequency...");
        for (uint16_t f = 10000; f <= 16000 && !stop; f += 1000) {
            i2sService.playTone(rate, f, 300);
            waitOrStop(100);
        }
        waitOrStop(800);
    }

    // Beep
    if (!stop) {
        terminalView.println("  Beep Pattern...");
        for (int i = 0; i < 3 && !stop; ++i) {
            i2sService.playTone(rate, 800, 100);
            waitOrStop(100);
        }
        for (int i = 0; i < 3 && !stop; ++i) {
            i2sService.playTone(rate, 800, 400);
            waitOrStop(100);
        }
        waitOrStop(800);
    }

    // Binary
    if (!stop) {
        terminalView.println("  Binary tone pattern...");
        for (int i = 0; i < 15 && !stop; ++i) {
            i2sService.playTone(rate, 1000, 50);
            waitOrStop(50);
        }
        waitOrStop(800);
    }

    // PCM
    if (!stop) {
        i2sService.configureOutput(
            state.getI2sBclkPin(),
            state.getI2sLrckPin(),
            state.getI2sDataPin(),
            24000,
            16,
            state.getI2sPercentLevel()
        );

        terminalView.println("  PCM playback...");
        i2sService.playPcm(PcmSoundtestComplete, sizeof(PcmSoundtestComplete));
    }

    // Restore config
    i2sService.configureOutput(
        state.getI2sBclkPin(),
        state.getI2sLrckPin(),
        state.getI2sDataPin(),
        state.getI2sSampleRate(),
        state.getI2sBitsPerSample(),
        state.getI2sPercentLevel()
    );

    if (stop)
        terminalView.println("\nI2S Speaker Test: Stopped by user.\n");
    else
        terminalView.println("\nI2S Speaker Test: Done.");
}

/*
Test Mic
*/
void I2sController::handleTestMic() {
    switchOutputInput(false);
    terminalView.println("\nI2S Micro: Analyzing input signal...\n");

    // Show pin config
    terminalView.println("Using GPIOs:");
    terminalView.println("  BCLK : " + std::to_string(state.getI2sBclkPin()));
    terminalView.println("  LRCK : " + std::to_string(state.getI2sLrckPin()));
    terminalView.println("  DATA : " + std::to_string(state.getI2sDataPin()));
    terminalView.println("");

    constexpr size_t sampleCount = 4096;
    std::vector<int16_t> buffer(sampleCount);
    size_t read = i2sService.recordSamples(buffer.data(), sampleCount);

    if (read == 0) {
        terminalView.println("\nI2S Micro: Failed to read from I2S microphone.");
        return;
    }

    int32_t sum = 0;
    int16_t minVal = INT16_MAX;
    int16_t maxVal = INT16_MIN;

    for (size_t i = 0; i < read; ++i) {
        int16_t val = buffer[i];
        sum += abs(val);
        if (val < minVal) minVal = val;
        if (val > maxVal) maxVal = val;
    }

    int avgAmplitude = sum / read;
    int peakToPeak = maxVal - minVal;

    // Score
    std::string verdict;
    if (avgAmplitude < 30 || peakToPeak < 60) {
        verdict = "Weak or no signal detected";
    } else if (avgAmplitude > 200 && peakToPeak > 400) {
        verdict = "Strong and valid signal";
    } else {
        verdict = "Low signal, maybe too quiet?";
    }

    // Display summay
    terminalView.println("Summary:");
    terminalView.println("  Avg amplitude : " + std::to_string(avgAmplitude));
    terminalView.println("  Min value     : " + std::to_string(minVal));
    terminalView.println("  Max value     : " + std::to_string(maxVal));
    terminalView.println("  Peak-to-peak  : " + std::to_string(peakToPeak));
    terminalView.println("  Verdict       : " + verdict);

    terminalView.println("\nI2S Micro: Done.");
}

/*
Config
*/
void I2sController::handleConfig() {
    terminalView.println("I2S Configuration:");

    const auto& forbidden = state.getProtectedPins();

    uint8_t bclk = userInputManager.readValidatedPinNumber("BCLK GPIO", state.getI2sBclkPin(), forbidden);
    state.setI2sBclkPin(bclk);

    uint8_t lrck = userInputManager.readValidatedPinNumber("LRCK/WS GPIO", state.getI2sLrckPin(), forbidden);
    state.setI2sLrckPin(lrck);

    uint8_t data = userInputManager.readValidatedPinNumber("DATA GPIO", state.getI2sDataPin(), forbidden);
    state.setI2sDataPin(data);

    uint32_t freq = userInputManager.readValidatedUint32("Sample rate (e.g. 44100)", state.getI2sSampleRate());
    state.setI2sSampleRate(freq);

    uint8_t bits = userInputManager.readValidatedUint8("Bits per sample (e.g. 16)", state.getI2sBitsPerSample());
    state.setI2sBitsPerSample(bits);

    uint8_t level = userInputManager.readValidatedUint8("Audio level (%)", state.getI2sPercentLevel(), 0, 100);
    state.setI2sPercentLevel(level);

    // Config will be applied at next play/record/test action
    terminalView.println("I2S configured.\n");
}

/*
Help
*/
void I2sController::handleHelp() {
    terminalView.println("\nUnknown command. Available I2S commands:");
    helpShell.run(state.getCurrentMode(), false);
}

/*
Reset
*/
void I2sController::handleReset() {
    switchOutputInput(true);
    terminalView.println("I2S Reset: TX (output) mode.");
}

/*
Ensure configuration
*/
void I2sController::ensureConfigured() {
    if (!configured) {
        handleConfig();
        configured = true;
    } else {
        // Reapply
        switchOutputInput(true);
    }
}

/*
Switch Output/Input
*/ 
void I2sController::switchOutputInput(bool output) {
    if (output) {
        i2sService.configureOutput(state.getI2sBclkPin(), state.getI2sLrckPin(),
                             state.getI2sDataPin(), state.getI2sSampleRate(),
                             state.getI2sBitsPerSample(), state.getI2sPercentLevel());
        if(!i2sService.isInitialized()){
            terminalView.println("I2S switchOutputInput: can't configure output .");
        }
    } else {
        i2sService.configureInput(state.getI2sBclkPin(), state.getI2sLrckPin(),
                            state.getI2sDataPin(), state.getI2sSampleRate(),
                            state.getI2sBitsPerSample());
        if(!i2sService.isInitialized()){
            terminalView.println("I2S switchOutputInput: can't configure input .");
        }
    }
}
