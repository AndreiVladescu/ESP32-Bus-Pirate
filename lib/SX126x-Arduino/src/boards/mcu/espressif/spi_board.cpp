#if defined ESP8266 || defined ESP32
#include <SPI.h>
#include "boards/mcu/espressif/spi_board.h"

static SPIClass* loraSpiInstance = nullptr;

void set_lora_spi_instance(SPIClass* spi)
{
    loraSpiInstance = spi;
}

SPIClass* get_lora_spi_instance()
{
    return loraSpiInstance;
}

void initSPI(void)
{
    // Bit Pirate owns SPI begin/end and pin mapping in LoRaService, exactly as
    // it does for the other external radio services. This library only uses
    // the injected instance and never initializes a default/display SPI bus.
}
#endif
