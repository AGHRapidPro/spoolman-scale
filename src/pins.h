#pragma once

/// Hardware pin assignments for the LOLIN S2 Mini.
/// Change these if you rewire the board.

// MFRC522 RFID reader (software SPI)
#define RFID_SS   13
#define RFID_RST  14
#define RFID_MISO 10
#define RFID_MOSI 11
#define RFID_SCK  12

// HX711 load-cell amplifier
#define HX711_DT  16
#define HX711_SCK 17
