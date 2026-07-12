#ifndef SPI_BOARD_H
#define SPI_BOARD_H

#include <SPI.h>

// Bit Pirate integration: the application owns the SPI bus and injects the
// exact SPIClass instance selected by the active board/device view.
// The SX126x library must never create or select a second SPI peripheral.
void set_lora_spi_instance(SPIClass* spi);
SPIClass* get_lora_spi_instance();

// Keep the upstream dot-based SPI_LORA usage without owning an SPIClass.
#define SPI_LORA (*get_lora_spi_instance())

void initSPI(void);

#endif // SPI_BOARD_H
