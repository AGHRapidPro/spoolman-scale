#include "Scale.h"

Scale::Scale(uint8_t dtPin, uint8_t sckPin)
    : _dtPin(dtPin), _sckPin(sckPin) {}

void Scale::begin() {
    _hx711.begin(_dtPin, _sckPin);
    float calib = _loadCalibration();
    _hx711.set_scale(calib);
    _hx711.tare(10);
    Serial.print("[Scale] Ready, calibration factor = "); Serial.println(calib, 3);
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
        Serial.print("  Stability: variation="); Serial.print(variation, 2); Serial.print(" g, target="); Serial.println(maxVariation, 2);

        if (variation < maxVariation) {
            float stable = (minVal + maxVal) / 2.0f;
            Serial.print("  Weight stable at "); Serial.print(stable, 2); Serial.print(" g after "); Serial.print(attempt + 1); Serial.println(" attempts");
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

    Serial.print("Calibration complete. New factor: "); Serial.print(newFactor, 3);
    Serial.print(", current reading: "); Serial.print(_hx711.get_units(), 1); Serial.println(" g");
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
