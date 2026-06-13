#include "CommandHandler.h"
#include <WiFi.h>

CommandHandler::CommandHandler(Scale& scale, RFIDReader& rfid,
                               SpoolmanClient& client, const Config& config)
    : _scale(scale), _rfid(rfid), _client(client), _config(config) {}

void CommandHandler::handle() {
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\r') continue;
        if (c == '\n') {
            _buf.trim();
            if (_buf.length() > 0) _dispatch(_buf);
            _buf = "";
            return; // one command per call to avoid long blocking
        }
        _buf += c;
    }
}

void CommandHandler::_dispatch(const String& raw) {
    String cmd = raw;
    cmd.toUpperCase();

    if (cmd == "WEIGHT") {
        Serial.printf("WEIGHT: %.2f g\n", _scale.readWeight(5));
    }
    else if (cmd == "TARE") {
        _scale.tare(10);
        Serial.println("TARE: done");
    }
    else if (cmd == "RFID") {
        const String& uid = _rfid.getUID();
        if (uid.length() > 0) Serial.printf("RFID: %s\n", uid.c_str());
        else                   Serial.println("RFID: none");
    }
    else if (cmd == "SPL_STATUS") {
        if (_config.spoolmanUrl().isEmpty()) {
            Serial.println("SPL_STATUS: none (no URL configured)");
        } else if (_client.checkStatus()) {
            Serial.println("SPL_STATUS: connected");
        } else {
            Serial.println("SPL_STATUS: none");
        }
    }
    else if (cmd == "WIFI_STATUS") {
        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("WIFI_STATUS: %s, IP: %s\n",
                          WiFi.SSID().c_str(),
                          WiFi.localIP().toString().c_str());
        } else {
            Serial.println("WIFI_STATUS: none");
        }
    }
    else if (cmd == "ONE_KG_SCALE") {
        _scale.calibrateInteractive();
    }
    else {
        Serial.printf("Unknown command: %s\n", cmd.c_str());
    }
}
