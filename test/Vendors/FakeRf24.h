#pragma once

enum rf24_datarate_e {
    RF24_1MBPS,
    RF24_2MBPS,
    RF24_250KBPS
};

enum rf24_crclength_e {
    RF24_CRC_DISABLED,
    RF24_CRC_8,
    RF24_CRC_16
};

enum rf24_pa_dbm_e {
    RF24_PA_MIN,
    RF24_PA_LOW,
    RF24_PA_HIGH,
    RF24_PA_MAX
};
