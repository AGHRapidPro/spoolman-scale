#include "Scale.h"

Scale::Scale(uint8_t dtPin, uint8_t sckPin)
    : _dtPin(dtPin), _sckPin(sckPin) {}

void Scale::begin() {
    _hx711.begin(_dtPin, _sckPin);
    float calib = _loadCalibration();
    _hx711.set_scale(calib);
    _hx711.tare(10);
    Serial.printf("[Scale] Ready, calibration factor = %.3f\n", calib);
}

void Scale::tare(int times) {
    _hx711.tare(times);
}

float Scale::readWeight(int samples) {
    float sum = 0;
    for (int i = 0; i < samples; i++) {
        sum += _hx711.get_units();
        delay(10);
    }
    return sum / samples;
}

float Scale::readStableWeight(int samples, float maxVariation) {
    const int bufSize = 5;
    float readings[bufSize] = {0};
    int idx = 0;

    for (int attempt = 0; attempt < 30; attempt++) {
        float sum = 0;
        for (int i = 0; i < samples; i++) {
            sum += _hx711.get_units();
            delay(10);
        }
        readings[idx] = sum / samples;
        idx = (idx + 1) % bufSize;

        float minVal = 9999, maxVal = -9999;
        for (int i = 0; i < bufSize; i++) {
            if (readings[i] < minVal) minVal = readings[i];
            if (readings[i] > maxVal) maxVal = readings[i];
        }
        float variation = maxVal - minVal;
        Serial.printf("  Stability: variation=%.2f g, target=%.2f\n", variation, maxVariation);

        if (variation < maxVariation) {
            float stable = (minVal + maxVal) / 2.0f;
            Serial.printf("  Weight stable at %.2f g after %d attempts\n", stable, attempt + 1);
            return stable;
        }
        delay(100);
    }

    Serial.println("  WARNING: Weight did not stabilise, using last reading.");
    return readings[0];
}

void Scale::calibrateInteractive() {
    Serial.println("1kg Calibration: Remove all weight, then press Enter.");
    while (!Serial.available()) { delay(100); }
    Serial.readStringUntil('\n');

    _hx711.tare(10);
    Serial.println("Tare done. Place exactly 1kg (1000g) weight on the scale, then press Enter.");
    while (!Serial.available()) { delay(100); }
    Serial.readStringUntil('\n');

    float sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += _hx711.get_units();
        delay(100);
    }
    float newFactor = (sum / 10.0f) / 1000.0f;
    _hx711.set_scale(newFactor);
    _saveCalibration(newFactor);

    Serial.printf("Calibration complete. New factor: %.3f, current reading: %.1f g\n",
                  newFactor, _hx711.get_units());
}

float Scale::_loadCalibration() {
    _prefs.begin("scale", true);
    float calib = _prefs.getFloat("calib", 1.0f);
    _prefs.end();
    return calib;
}

void Scale::_saveCalibration(float factor) {
    _prefs.begin("scale", false);
    _prefs.putFloat("calib", factor);
    _prefs.end();
}
