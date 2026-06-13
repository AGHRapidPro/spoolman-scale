#include "RFIDReader.h"

RFIDReader::RFIDReader(uint8_t ssPin, uint8_t rstPin,
                       uint8_t sckPin, uint8_t misoPin, uint8_t mosiPin)
    : _mfrc522(ssPin, rstPin), _ssPin(ssPin), _sckPin(sckPin), _misoPin(misoPin), _mosiPin(mosiPin) {}

void RFIDReader::begin() {
    SPI.begin(_sckPin, _misoPin, _mosiPin, _ssPin);
    _mfrc522.PCD_Init();
    Serial.println("[RFID] MFRC522 initialized");
}

bool RFIDReader::poll() {
    if (!_mfrc522.PICC_IsNewCardPresent()) return false;
    if (!_mfrc522.PICC_ReadCardSerial())   return false;

    _uid = "";
    for (byte i = 0; i < _mfrc522.uid.size; i++) {
        if (_mfrc522.uid.uidByte[i] < 0x10) _uid += "0";
        _uid += String(_mfrc522.uid.uidByte[i], HEX);
    }
    return true;
}

void RFIDReader::halt() {
    _mfrc522.PICC_HaltA();
}
