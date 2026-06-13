#include "SpoolmanClient.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

SpoolmanClient::SpoolmanClient(const Config& config) : _config(config) {}

String SpoolmanClient::_baseUrl() {
    String url = _config.spoolmanUrl();
    if (!url.endsWith("/")) url += "/";
    return url;
}

bool SpoolmanClient::_refreshCache() {
    //Load all spools at startup (to later get RFID since API for that is not available)
    HTTPClient http;
    String url = _baseUrl() + "spool";
    Serial.print("  [cache] GET "); Serial.println(url);
    http.begin(url);

    int code = http.GET();
    if (code != 200) {
        http.end();
        Serial.print("  [cache] fetch failed: "); Serial.println(code);
        return false;
    }

    DynamicJsonDocument doc(8192);
    DeserializationError err = deserializeJson(doc, http.getStream());
    http.end();
    if (err) {
        Serial.println("  [cache] JSON parse failed");
        return false;
    }

    _rfidCache.clear();
    for (JsonObject spool : doc.as<JsonArray>()) {
        const char* rawRfid = spool["extra"]["rfid"];
        if (!rawRfid) continue;
        String uid = String(rawRfid);
        uid.replace("\"", "");
        int id = spool["id"];
        _rfidCache[uid] = id;
        Serial.print("  [cache] rfid="); Serial.print(uid); Serial.print(" -> spool "); Serial.println(id);
    }

    _cacheLoaded = true;
    Serial.print("  [cache] loaded "); Serial.print(_rfidCache.size()); Serial.println(" entries");
    return true;
}

bool SpoolmanClient::fetchSpoolByRFID(const String& uid, SpoolInfo& out) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("  WiFi not connected");
        return false;
    }

    // Load cache if empty, or retry on miss
    if (!_cacheLoaded) _refreshCache();

    auto it = _rfidCache.find(uid);
    if (it == _rfidCache.end()) {
        // One retry with fresh cache in case spool was just added
        Serial.println("  [cache] miss, refreshing...");
        _refreshCache();
        it = _rfidCache.find(uid);
        if (it == _rfidCache.end()) {
            Serial.println("  No spool found for this RFID.");
            return false;
        }
    }

    int spoolId = it->second;
    HTTPClient http;
    String url = _baseUrl() + "spool/" + String(spoolId);
    Serial.print("  GET "); Serial.println(url);
    http.begin(url);

    int code = http.GET();
    if (code != 200) {
        http.end();
        return false;
    }

    DynamicJsonDocument doc(2048);
    DeserializationError err = deserializeJson(doc, http.getStream());
    http.end();
    if (err) return false;

    out.id            = doc["id"];
    out.emptyWeight   = doc["spool_weight"]         | 0.0f;
    out.filamentTotal = doc["filament"]["weight"]   | 0.0f;
    out.usedWeight    = doc["used_weight"]          | 0.0f;
    Serial.print("  Spool ID="); Serial.print(out.id);
    Serial.print(", empty="); Serial.print(out.emptyWeight, 1);
    Serial.print(" g, total="); Serial.print(out.filamentTotal, 1);
    Serial.print(" g, used="); Serial.print(out.usedWeight, 1); Serial.println(" g");
    return true;
}

bool SpoolmanClient::updateSpoolWeight(int spoolId, float filamentWeight, float filamentTotal) {
    float newUsed = filamentTotal - filamentWeight;
    if (newUsed < 0) newUsed = 0;

    HTTPClient http;
    String url = _baseUrl() + "spool/" + String(spoolId);
    Serial.print("  PATCH "); Serial.println(url);
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    DynamicJsonDocument doc(256);
    doc["used_weight"] = newUsed;
    String body;
    serializeJson(doc, body);
    Serial.print("  Body: "); Serial.println(body);

    int code = http.PATCH(body);
    Serial.print("  HTTP response code: "); Serial.println(code);
    http.end();
    return (code == 200);
}

bool SpoolmanClient::checkStatus() {
    if (_config.spoolmanUrl().isEmpty() || WiFi.status() != WL_CONNECTED) return false;

    HTTPClient http;
    http.begin(_baseUrl());
    http.setTimeout(5000);
    int code = http.GET();
    http.end();
    return (code > 0);
}
