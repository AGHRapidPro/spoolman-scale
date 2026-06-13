#pragma once
#include <Arduino.h>
#include <HTTPClient.h>
#include "Config.h"
#include <map>

/// Data returned by the Spoolman API for a single spool.
struct SpoolInfo {
    int   id            = 0;
    float emptyWeight   = 0.0f;  ///< weight of the empty spool in grams
    float filamentTotal = 0.0f;  ///< total filament weight when full, in grams
    float usedWeight    = 0.0f;  ///< filament already consumed, in grams
};

/// HTTP client for the Spoolman REST API.
/// Reads URL and API key from Config on every request, so changes
/// made through WiFiManager take effect immediately.
class SpoolmanClient {
public:
    explicit SpoolmanClient(const Config& config);

    /// Looks up a spool by RFID UID via GET /spool?rfid=<uid>.
    /// Populates out on success. Returns false if WiFi is down or the server errors.
    bool fetchSpoolByRFID(const String& uid, SpoolInfo& out);

    /// Calculates used_weight = filamentTotal - filamentWeight and PATCHes the spool.
    /// filamentWeight is what the scale measured minus the empty spool weight.
    bool updateSpoolWeight(int spoolId, float filamentWeight, float filamentTotal);

    /// Quick GET to the base URL to verify the server is reachable.
    bool checkStatus();

private:
    const Config& _config;
    std::map<String, int> _rfidCache;  // rfid uid → spool id
    bool _cacheLoaded = false;

    bool        _refreshCache();
    String      _baseUrl();
};
