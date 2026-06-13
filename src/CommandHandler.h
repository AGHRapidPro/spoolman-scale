#pragma once
#include <Arduino.h>
#include "Scale.h"
#include "RFIDReader.h"
#include "SpoolmanClient.h"
#include "Config.h"

/// Reads Serial line by line and dispatches commands to the relevant subsystem.
///
/// Supported commands (case-insensitive):
///   WEIGHT       — print current scale reading
///   TARE         — tare the scale
///   RFID         — print last detected UID
///   SPL_STATUS   — check Spoolman server reachability
///   WIFI_STATUS  — print SSID and IP
///   ONE_KG_SCALE — start interactive 1 kg calibration
class CommandHandler {
public:
    CommandHandler(Scale& scale, RFIDReader& rfid,
                   SpoolmanClient& client, const Config& config);

    /// Call every iteration of loop(). Non-blocking — returns after processing
    /// at most one complete newline-terminated command per call.
    void handle();

private:
    void _dispatch(const String& cmd);

    Scale&          _scale;
    RFIDReader&     _rfid;
    SpoolmanClient& _client;
    const Config&   _config;
    String          _buf;
};
