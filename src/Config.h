#pragma once
#include <Arduino.h>
#include <Preferences.h>

/// Manages persistent configuration — Spoolman URL and API key.
/// Values are stored in NVS flash and optionally updated through the
/// WiFiManager captive portal on first boot or when WiFi is lost.
class Config {
public:
    Config();

    /// Reads URL and API key from flash. Call once in setup() before anything else.
    void load();

    /// Connects to WiFi. If no credentials are saved, or the connection fails,
    /// opens the "SpoolmanScale" captive portal so the user can configure
    /// WiFi and the Spoolman URL. Blocks until connected or the portal times out.
    void startWiFiManager();

    const String& spoolmanUrl() const { return _url; }

private:

    #ifndef PASSWORDS
    String      _url = "https://spoolman.aghrapid.top/settings"; //not aplicable for now
    #elif

    #endif
    Preferences _prefs;

    /// Writes current URL and API key to NVS flash.
    void _persist();
};
