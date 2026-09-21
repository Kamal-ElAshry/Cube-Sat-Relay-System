#include "ina219.h"
#include "config.h"

// The Adafruit INA219 library. Its begin() call also handles the shared
// I2C bus init internally (same pattern already relied on for the BMP180
// via Adafruit_BMP085) - no separate Wire.begin() needed here.
#include <Adafruit_INA219.h>
#include <Arduino.h> // millis()

static Adafruit_INA219 _ina219(INA219_I2C_ADDRESS);
static bool _available = false;
static unsigned long _lastRetryMs = 0;

static void _applyCalibration() {
    // See config.h for what each preset assumes about the shunt resistor
    // and measurable range - this must match your actual breakout board.
    switch (INA219_CALIBRATION) {
        case Ina219Calibration::RANGE_32V_1A:
            _ina219.setCalibration_32V_1A();
            break;
        case Ina219Calibration::RANGE_16V_400MA:
            _ina219.setCalibration_16V_400mA();
            break;
        case Ina219Calibration::RANGE_32V_2A:
        default:
            _ina219.setCalibration_32V_2A();
            break;
    }
}

bool ina219_init() {
    // NOTE: current Adafruit_INA219 library versions have begin() return
    // bool; older versions returned void. If you're pinned to an old
    // version, this line needs a one-line adjustment (same caveat already
    // applies to RF24::begin() elsewhere in this project).
    _available = _ina219.begin();
    if (_available) {
        _applyCalibration();
    }
    return _available;
}

bool ina219_isAvailable() {
    return _available;
}

void ina219_maintain() {
    if (_available) {
        return;
    }
    unsigned long now = millis();
    if (now - _lastRetryMs >= INA219_RETRY_INTERVAL_MS) {
        _lastRetryMs = now;
        ina219_init(); // non-blocking: one attempt, success or not, loop() continues either way
    }
}

bool ina219_read(int32_t& current_mA, uint16_t& busVoltage_mV, uint32_t& power_mW) {
    if (!_available) {
        return false;
    }

    float current = _ina219.getCurrent_mA();
    float busV_V  = _ina219.getBusVoltage_V();
    float power   = _ina219.getPower_mW();

    // current_mA is signed (current can flow in either direction depending
    // on shunt orientation) - preserved as-is.
    current_mA = (int32_t)current;

    float busV_mV = busV_V * 1000.0f;
    if (busV_mV < 0.0f) busV_mV = 0.0f; // bus voltage is not physically negative
    busVoltage_mV = (uint16_t)busV_mV;

    // power_mW is unsigned per the packet spec, while current_mA already
    // carries directionality - clamp rather than let a negative float wrap
    // into a huge value when cast to uint32_t.
    if (power < 0.0f) power = 0.0f;
    power_mW = (uint32_t)power;

    return true;
}
