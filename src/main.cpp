#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <HX711.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>

// ==================== PIN DEFINITIONS (LOLIN S2 Mini) ====================
#define RFID_SS   13 // SDA
#define RFID_RST  14
#define RFID_MISO 10
#define RFID_MOSI 11
#define RFID_SCK  12

#define HX711_DT  18
#define HX711_SCK 21

// ==================== GLOBALS ====================
MFRC522 mfrc522(RFID_SS, RFID_RST);
HX711 scale;
Preferences prefs;

String spoolmanUrl;
String apiKey;

// ==================== WEIGHT STABILISATION ====================
float readStableWeight(int samples = 5, float maxVariation = 1.5f) {
    const int bufSize = 5;
    float readings[bufSize] = {0};
    int idx = 0;

    for (int attempt = 0; attempt < 30; attempt++) {
        float sum = 0;
        for (int i = 0; i < samples; i++) {
            sum += scale.get_units();
            delay(10);
        }
        float newVal = sum / samples;
        readings[idx] = newVal;
        idx = (idx + 1) % bufSize;

        float minVal = 9999, maxVal = 0;
        for (int i = 0; i < bufSize; i++) {
            if (readings[i] < minVal) minVal = readings[i];
            if (readings[i] > maxVal) maxVal = readings[i];
        }
        float variation = maxVal - minVal;
        Serial.printf("  Stability: variation=%.2f g, target=%.2f\n", variation, maxVariation);
        if (variation < maxVariation) {
            float stable = (minVal + maxVal) / 2;
            Serial.printf("  Weight stable at %.2f g after %d attempts\n", stable, attempt+1);
            return stable;
        }
        delay(100);
    }
    Serial.println("  WARNING: Weight did not stabilise, using last reading.");
    return readings[0];
}

// ==================== SPOOLMAN API ====================
bool fetchSpoolByRFID(const String& uid, int& spoolId, float& emptyWeight,
                      float& filamentTotal, float& usedWeight) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("  WiFi not connected, cannot fetch spool.");
        return false;
    }

    HTTPClient http;
    String url = spoolmanUrl;
    if (!url.endsWith("/")) url += "/";
    url += "spool?rfid=" + uid;
    Serial.print("  GET "); Serial.println(url);
    http.begin(url);
    if (!apiKey.isEmpty()) {
        http.addHeader("X-Api-Key", apiKey);
        Serial.println("  Added API key header");
    }

    int code = http.GET();
    Serial.printf("  HTTP response code: %d\n", code);
    if (code != 200) {
        http.end();
        return false;
    }

    DynamicJsonDocument doc(1024);
    DeserializationError err = deserializeJson(doc, http.getStream());
    http.end();
    if (err) {
        Serial.println("  JSON parsing failed");
        return false;
    }

    spoolId = doc["id"];
    emptyWeight = doc["empty_weight"] | 0.0f;
    filamentTotal = doc["filament"]["weight"] | 0.0f;
    usedWeight = doc["used_weight"] | 0.0f;
    Serial.printf("  Spool ID=%d, empty=%.1f g, filament total=%.1f g, used=%.1f g\n",
                  spoolId, emptyWeight, filamentTotal, usedWeight);
    return true;
}

bool updateSpoolWeight(int spoolId, float newFilamentWeight, float filamentTotal) {
    float newUsed = filamentTotal - newFilamentWeight;
    if (newUsed < 0) newUsed = 0;

    HTTPClient http;
    String url = spoolmanUrl;
    if (!url.endsWith("/")) url += "/";
    url += "spool/" + String(spoolId);
    Serial.print("  PATCH "); Serial.println(url);
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    if (!apiKey.isEmpty()) http.addHeader("X-Api-Key", apiKey);

    DynamicJsonDocument doc(256);
    doc["used_weight"] = newUsed;
    String body;
    serializeJson(doc, body);
    Serial.print("  Body: "); Serial.println(body);

    int code = http.PATCH(body);
    Serial.printf("  HTTP response code: %d\n", code);
    http.end();
    return (code == 200);
}

// ==================== SETUP ====================
void setup() {
    Serial.begin(115200);
    delay(1000); // allow serial monitor to connect
    Serial.println("\n\n========================================");
    Serial.println("Spoolman Headless Scale (LOLIN S2 Mini)");
    Serial.println("========================================\n");

    // ---- RFID ----
    SPI.begin(RFID_SCK, RFID_MISO, RFID_MOSI, RFID_SS);
    mfrc522.PCD_Init();
    Serial.println("[RFID] MFRC522 initialized");

    // ---- HX711 ----
    scale.begin(HX711_DT, HX711_SCK);
    prefs.begin("scale", true);
    float calib = prefs.getFloat("calib", 1.0f);
    prefs.end();
    scale.set_scale(calib);
    scale.tare(10);
    Serial.printf("[Scale] Ready, calibration factor = %.3f\n", calib);

    // ---- Load saved Spoolman config ----
    prefs.begin("spoolman", true);
    spoolmanUrl = prefs.getString("url", "");
    apiKey = prefs.getString("apikey", "");
    prefs.end();
    if (spoolmanUrl.length() > 0) {
        Serial.printf("[Config] Loaded Spoolman URL: %s\n", spoolmanUrl.c_str());
        if (apiKey.length()) Serial.println("[Config] API key loaded (hidden)");
    } else {
        Serial.println("[Config] No saved Spoolman URL.");
    }

    // ---- WiFiManager ----
    if (spoolmanUrl.isEmpty() || WiFi.status() != WL_CONNECTED) {
        Serial.println("[WiFi] Starting configuration portal...");
        WiFiManager wm;
        WiFiManagerParameter custom_url("url", "Spoolman URL (e.g. http://192.168.1.100:7912/api/v1)", spoolmanUrl.c_str(), 100);
        WiFiManagerParameter custom_apikey("apikey", "API Key (optional)", apiKey.c_str(), 64);
        wm.addParameter(&custom_url);
        wm.addParameter(&custom_apikey);

        wm.setSaveParamsCallback([&]() {
            spoolmanUrl = custom_url.getValue();
            apiKey = custom_apikey.getValue();
            prefs.begin("spoolman", false);
            prefs.putString("url", spoolmanUrl);
            prefs.putString("apikey", apiKey);
            prefs.end();
            Serial.println("[WiFi] Spoolman config saved.");
        });

        wm.autoConnect("SpoolmanScale");
        Serial.println("[WiFi] Configuration finished.");
    }

    // Print final network status
    Serial.print("[WiFi] Connected to ");
    Serial.println(WiFi.SSID());
    Serial.print("[WiFi] IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("[WiFi] Spoolman URL: ");
    Serial.println(spoolmanUrl);
    Serial.println("\nReady. Place an RFID-tagged spool on the scale.\n");
}

// ==================== MAIN LOOP ====================
void loop() {
    // Wait for a new RFID tag
    if (!mfrc522.PICC_IsNewCardPresent()) {
        delay(100);
        return;
    }
    if (!mfrc522.PICC_ReadCardSerial()) {
        delay(100);
        return;
    }

    // Read UID as hex string
    String uid = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
        if (mfrc522.uid.uidByte[i] < 0x10) uid += "0";
        uid += String(mfrc522.uid.uidByte[i], HEX);
    }
    Serial.println("\n========================================");
    Serial.print("[RFID] Tag detected, UID: ");
    Serial.println(uid);
    mfrc522.PICC_HaltA();

    // Wait for stable weight
    Serial.println("[Scale] Waiting for stable weight...");
    float measuredTotal = readStableWeight(5, 1.5f);
    Serial.printf("[Scale] Measured total weight: %.2f g\n", measuredTotal);

    // Fetch spool from Spoolman
    Serial.println("[Spoolman] Fetching spool data...");
    int spoolId;
    float emptyWeight, filamentTotal, usedWeight;
    if (!fetchSpoolByRFID(uid, spoolId, emptyWeight, filamentTotal, usedWeight)) {
        Serial.println("[ERROR] Spool not found or server unreachable.");
        delay(2000);
        return;
    }

    float filamentWeight = measuredTotal - emptyWeight;
    if (filamentWeight < 0) filamentWeight = 0;
    Serial.printf("[Calc] Filament weight = %.2f g (total - empty)\n", filamentWeight);

    // Update server
    Serial.println("[Spoolman] Updating weight...");
    if (updateSpoolWeight(spoolId, filamentWeight, filamentTotal)) {
        Serial.println("[SUCCESS] Weight updated on server.\n");
    } else {
        Serial.println("[ERROR] Failed to update weight.\n");
    }

    delay(3000); //szmeges
}

// ==================== CALIBRATION (uncomment and run once) ====================
/*
void calibrateScale() {
    Serial.println("Remove all weight, press Enter");
    while (!Serial.available()) delay(100);
    Serial.readString();
    scale.tare(10);
    Serial.println("Tare done. Place known weight (e.g. 100g) and enter its value:");
    while (!Serial.available()) delay(100);
    float known = Serial.readString().toFloat();
    float raw = 0;
    for (int i=0;i<10;i++) raw += scale.get_units();
    raw /= 10;
    float newFactor = raw / known;
    scale.set_scale(newFactor);
    prefs.begin("scale", false);
    prefs.putFloat("calib", newFactor);
    prefs.end();
    Serial.printf("Calibration factor set to %.3f\n", newFactor);
}
*/