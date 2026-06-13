#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>

/// Wraps the MFRC522 RFID reader over software SPI.
class RFIDReader {
public:
    RFIDReader(uint8_t ssPin, uint8_t rstPin,
               uint8_t sckPin, uint8_t misoPin, uint8_t mosiPin);

    /// Initialises SPI and the MFRC522 chip.
    void begin();

    /// Returns true if a new tag was detected and its UID read successfully.
    /// Call getUID() immediately after — the value is overwritten on the next poll().
    bool poll();

    /// Halts the active tag so it won't be detected again until removed and re-presented.
    void halt();

    const String& getUID() const { return _uid; }

private:
    MFRC522 _mfrc522;
    uint8_t _ssPin;
    uint8_t _sckPin;
    uint8_t _misoPin;
    uint8_t _mosiPin;
    String  _uid;
};
