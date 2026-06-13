#pragma once
#include <Arduino.h>
#include <HX711.h>
#include <Preferences.h>

/// Wraps the HX711 load-cell amplifier.
/// Handles tare, calibration persistence, and stabilised weight readings.
class Scale {
public:
    Scale(uint8_t dtPin, uint8_t sckPin);

    /// Initialises the HX711, loads the saved calibration factor, and tares.
    void begin();

    void tare(int times = 10);

    /// Returns the weight in grams averaged over N samples.
    float readWeight(int samples = 5);

    /// Repeatedly samples until N consecutive readings agree within
    /// maxVariation grams, then returns the midpoint. Falls back to the
    /// last reading after 30 attempts.
    float readStableWeight(int samples = 5, float maxVariation = 1.5f);

    /// Interactive 1 kg calibration over Serial. Prompts the user to remove
    /// weight, tares, then prompts to place 1000 g and computes the factor.
    /// Saves the result to flash. Blocks until complete.
    void calibrateInteractive();

private:
    HX711       _hx711;
    uint8_t     _dtPin;
    uint8_t     _sckPin;
    Preferences _prefs;

    void  _saveCalibration(float factor);
    float _loadCalibration();
};
