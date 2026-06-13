#include "Config.h"
#include <WiFiManager.h>
#include <WiFi.h>

Config::Config() {}

void Config::load() {
    _prefs.begin("spoolman", true);
    _url    = _prefs.getString("url",    "");
    _prefs.end();

    if (_url.length() > 0) {
        Serial.print("[Config] Loaded Spoolman URL: "); Serial.println(_url);
    } 
    else {
        Serial.println("[Config] No saved Spoolman URL.");
    }
}

void Config::startWiFiManager() {
    if (!_url.isEmpty() && WiFi.status() == WL_CONNECTED) return;

    Serial.println("[WiFi] Starting configuration portal...");
    WiFiManager wm;
    WiFiManagerParameter paramUrl("url", "Spoolman URL (e.g. http://192.168.1.100:7912/api/v1)", _url.c_str(), 100);
    wm.addParameter(&paramUrl);

    wm.setSaveParamsCallback([&]() {
        _url    = paramUrl.getValue();
        _persist();
        Serial.println("[WiFi] Spoolman config saved.");
    });

    wm.autoConnect("SpoolmanScale");
    Serial.println("[WiFi] Configuration finished.");
}

void Config::_persist() {
    _prefs.begin("spoolman", false);
    _prefs.putString("url",    _url);
    _prefs.end();
}
