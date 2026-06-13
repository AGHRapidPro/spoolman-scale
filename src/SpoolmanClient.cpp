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

// void SpoolmanClient::_addAuthHeader(HTTPClient& http) {
//     if (!_config.apiKey().isEmpty()) {
//         http.addHeader("X-Api-Key", _config.apiKey());
//     }
// }

bool SpoolmanClient::fetchSpoolByRFID(const String& uid, SpoolInfo& out) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("  WiFi not connected, cannot fetch spool.");
        return false;
    }

    HTTPClient http;
    String url = _baseUrl() + "spool?rfid=" + uid;
    Serial.print("  GET "); Serial.println(url);
    http.begin(url);
    //_addAuthHeader(http);

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

    out.id            = doc["id"];
    out.emptyWeight   = doc["empty_weight"]       | 0.0f;
    out.filamentTotal = doc["filament"]["weight"]  | 0.0f;
    out.usedWeight    = doc["used_weight"]         | 0.0f;
    Serial.printf("  Spool ID=%d, empty=%.1f g, filament total=%.1f g, used=%.1f g\n",
                  out.id, out.emptyWeight, out.filamentTotal, out.usedWeight);
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
    //_addAuthHeader(http);

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

bool SpoolmanClient::checkStatus() {
    if (_config.spoolmanUrl().isEmpty() || WiFi.status() != WL_CONNECTED) return false;

    HTTPClient http;
    http.begin(_baseUrl());
    //_addAuthHeader(http);
    http.setTimeout(5000);
    int code = http.GET();
    http.end();
    return (code > 0);
}
