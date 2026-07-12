#include "PinoutTransformer.h"


PinoutConfig PinoutTransformer::build(ModeEnum mode) const {
    PinoutConfig config;
    config.setMode(ModeEnumMapper::toString(mode));

    switch (mode) {
        case ModeEnum::OneWire:
            config.setMappings({ "DATA GPIO " + std::to_string(state.getOneWirePin()) });
            break;

        case ModeEnum::UART:
            config.setMappings({
                "TX GPIO " + std::to_string(state.getUartTxPin()),
                "RX GPIO " + std::to_string(state.getUartRxPin()),
                "BAUD " + std::to_string(state.getUartBaudRate()),
                "BITS " + std::to_string(state.getUartDataBits()),
            });
            break;

        case ModeEnum::HDUART:
            config.setMappings({
                "RX/TX GPIO " + std::to_string(state.getHdUartPin()),
                "BAUD " + std::to_string(state.getHdUartBaudRate()),
                "BITS " + std::to_string(state.getHdUartDataBits()),
                "PARITY " + state.getHdUartParity(),
            });
            break;

        case ModeEnum::I2C:
            config.setMappings({
                "SDA GPIO " + std::to_string(state.getI2cSdaPin()),
                "SCL GPIO " + std::to_string(state.getI2cSclPin()),
                "FREQ " + std::to_string(state.getI2cFrequency())
            });
            break;

        case ModeEnum::SPI:
            config.setMappings({
                "MOSI GPIO " + std::to_string(state.getSpiMOSIPin()),
                "MISO GPIO " + std::to_string(state.getSpiMISOPin()),
                "SCLK GPIO " + std::to_string(state.getSpiCLKPin()),
                "CS GPIO " + std::to_string(state.getSpiCSPin())
            });
            break;

        case ModeEnum::TwoWire:
            config.setMappings({
                "DATA GPIO " + std::to_string(state.getTwoWireIoPin()),
                "CLK GPIO " + std::to_string(state.getTwoWireClkPin()),
                "RST GPIO " + std::to_string(state.getTwoWireRstPin())
            });
            break;

        case ModeEnum::ThreeWire:
            config.setMappings({
                "CS GPIO " + std::to_string(state.getThreeWireCsPin()),
                "SK GPIO " + std::to_string(state.getThreeWireSkPin()),
                "DI GPIO " + std::to_string(state.getThreeWireDiPin()),
                "DO GPIO " + std::to_string(state.getThreeWireDoPin())
            });
            break;

        case ModeEnum::LED:
            config.setMappings({
                "DATA GPIO " + std::to_string(state.getLedDataPin()),
                "CLOCK GPIO " + std::to_string(state.getLedClockPin()),
                "LED COUNT " + std::to_string(state.getLedLength()),
                state.getLedProtocol()
            });
            break;

        case ModeEnum::Infrared: {
            auto proto = InfraredProtocolMapper::toString(state.getInfraredProtocol());
            config.setMappings({
                "IR TX GPIO " + std::to_string(state.getInfraredTxPin()),
                "IR RX GPIO " + std::to_string(state.getInfraredRxPin()),
                proto
            });
            break;
        }

        case ModeEnum::JTAG: {
            std::vector<std::string> lines;
            const auto& pins = state.getJtagScanPins();
            size_t totalPins = pins.size();

            for (size_t i = 0; i < std::min(totalPins, size_t(4)); ++i) {
                std::string line = "SCAN GPIO " + std::to_string(pins[i]);
                if (i == 3 && totalPins > 4) line += " ...";
                lines.push_back(line);
            }
            config.setMappings(lines);
            break;
        }

        case ModeEnum::I2S:
            config.setMappings({
                "BCLK GPIO " + std::to_string(state.getI2sBclkPin()),
                "LRCLK GPIO " + std::to_string(state.getI2sLrckPin()),
                "DATA GPIO " + std::to_string(state.getI2sDataPin()),
                "RATE " + std::to_string(state.getI2sSampleRate())
            });
            break;

        case ModeEnum::CAN_:
            config.setMappings({
                "CS GPIO " + std::to_string(state.getCanCspin()),
                "SCK GPIO " + std::to_string(state.getCanSckPin()),
                "SI GPIO " + std::to_string(state.getCanSiPin()),
                "SO GPIO " + std::to_string(state.getCanSoPin())
            });
            break;

        case ModeEnum::ETHERNET:
            config.setMappings({
                "CS GPIO " + std::to_string(state.getEthernetCsPin()),
                "SCK GPIO " + std::to_string(state.getEthernetSckPin()),
                "SI GPIO " + std::to_string(state.getEthernetMosiPin()),
                "SO GPIO " + std::to_string(state.getEthernetMisoPin())
            });
            break;

        case ModeEnum::SUBGHZ:
            config.setMappings({
                "SCK GPIO " + std::to_string(state.getSubGhzSckPin()),
                "MISO GPIO " + std::to_string(state.getSubGhzMisoPin()),
                "MOSI GPIO " + std::to_string(state.getSubGhzMosiPin()),
                "CS GPIO " + std::to_string(state.getSubGhzCsPin()),
            });
            break;

        case ModeEnum::RFID:
            config.setMappings({
                "RFID SDA GPIO " + std::to_string(state.getRfidSdaPin()),
                "RFID SCL GPIO " + std::to_string(state.getRfidSclPin())
            });
            break;

        case ModeEnum::RF24_:
            config.setMappings({
                "CE GPIO " + std::to_string(state.getRf24CePin()),
                "CSN GPIO " + std::to_string(state.getRf24CsnPin()),
                "SCK GPIO " + std::to_string(state.getRf24SckPin()),
                "MOSI GPIO " + std::to_string(state.getRf24MosiPin())
            });
            break;

        case ModeEnum::FM:
            config.setMappings({
                "SDA GPIO " + std::to_string(state.getTwoWireIoPin()),
                "SCL GPIO " + std::to_string(state.getTwoWireClkPin()),
                "RST GPIO " + std::to_string(state.getTwoWireRstPin()),
                "Module Si4713"
            });
            break;

        case ModeEnum::CELL:
            config.setMappings({
                "RX GPIO " + std::to_string(state.getUartRxPin()),
                "TX GPIO " + std::to_string(state.getUartTxPin()),
                "BAUD " + std::to_string(state.getUartBaudRate()),
            });
            break;
        case ModeEnum::LORA:
            config.setMappings({
                "SCK GPIO " + std::to_string(state.getLoRaSckPin()),
                "MISO GPIO " + std::to_string(state.getLoRaMisoPin()),
                "MOSI GPIO " + std::to_string(state.getLoRaMosiPin()),
                "CS GPIO " + std::to_string(state.getLoRaCsPin())
            });
            break;
            
        default:
            break;
    }

    return config;
}
