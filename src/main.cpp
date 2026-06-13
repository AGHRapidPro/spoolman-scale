#include <Arduino.h>
#include <WiFi.h>
#include "pins.h"
#include "Config.h"
#include "Scale.h"
#include "RFIDReader.h"
#include "SpoolmanClient.h"
#include "CommandHandler.h"

Config          config;
Scale           scale(HX711_DT, HX711_SCK);
RFIDReader      rfid(RFID_SS, RFID_RST, RFID_SCK, RFID_MISO, RFID_MOSI);
SpoolmanClient  spoolman(config);
CommandHandler  commands(scale, rfid, spoolman, config);

void setup() {
    Serial.begin(115200);
    delay(10000);
    Serial.println("\n\n========================================");
    Serial.println("Spoolman Headless Scale (LOLIN S2 Mini)");
    Serial.println("========================================\n");

    rfid.begin();
    scale.begin();
    config.load();
    config.startWiFiManager();

    Serial.print("[WiFi] Connected to "); Serial.println(WiFi.SSID());
    Serial.print("[WiFi] IP address: ");  Serial.println(WiFi.localIP());
    Serial.print("[WiFi] Spoolman URL: "); Serial.println(config.spoolmanUrl());
    Serial.println("\nReady. Place an RFID-tagged spool on the scale.\n");
}

void loop() {
    commands.handle();

    if (!rfid.poll()) {
        unsigned long start = millis();
        while (!rfid.poll() && (millis() - start < 100)) {
            commands.handle();
            delay(1);
        }
        return;
    }

    Serial.println("\n========================================");
    Serial.printf("[RFID] Tag detected, UID: %s\n", rfid.getUID().c_str());
    rfid.halt();

    Serial.println("[Scale] Waiting for stable weight...");
    float measuredTotal = scale.readStableWeight(5, 1.5f);
    Serial.printf("[Scale] Measured total weight: %.2f g\n", measuredTotal);

    Serial.println("[Spoolman] Fetching spool data...");
    SpoolInfo spool;
    if (!spoolman.fetchSpoolByRFID(rfid.getUID(), spool)) {
        Serial.println("[ERROR] Spool not found or server unreachable.");
        unsigned long t = millis();
        while (millis() - t < 3000) { commands.handle(); delay(10); }
        return;
    }

    float filamentWeight = measuredTotal - spool.emptyWeight;
    if (filamentWeight < 0) filamentWeight = 0;
    Serial.printf("[Calc] Filament weight = %.2f g (total - empty)\n", filamentWeight);

    Serial.println("[Spoolman] Updating weight...");
    if (spoolman.updateSpoolWeight(spool.id, filamentWeight, spool.filamentTotal)) {
        Serial.println("[SUCCESS] Weight updated on server.\n");
    } else {
        Serial.println("[ERROR] Failed to update weight.\n");
    }

    unsigned long t = millis();
    while (millis() - t < 3000) { commands.handle(); delay(10); }
}
