#include "barometer.h"
#include "config.h"

// The Adafruit_BMP085 library is register-compatible with the BMP180 and
// is the library recommended by Adafruit for this exact sensor.
#include <Adafruit_BMP085.h>
#include <Arduino.h> // millis()

static Adafruit_BMP085 _bmp;
static bool _available = false;
static unsigned long _lastRetryMs = 0;

bool barometer_init() {
    _available = _bmp.begin(); // fixed I2C address 0x77, handled internally by the library
    return _available;
}

void barometer_maintain() {
    if (_available) {
        return;
    }
    unsigned long now = millis();
    if (now - _lastRetryMs >= BAROMETER_RETRY_INTERVAL_MS) {
        _lastRetryMs = now;
        barometer_init(); // non-blocking: one attempt, success or not, loop() continues either way
    }
}

bool barometer_isAvailable() {
    return _available;
}

bool barometer_read(int16_t& tempC_x10, uint32_t& pressurePa, int16_t& altitudeM_x10) {
    if (!_available) {
        return false;
    }

    float   t    = _bmp.readTemperature();
    int32_t p    = _bmp.readPressure();
    float   alt  = _bmp.readAltitude(BMP_REFERENCE_PRESSURE_PA);

    tempC_x10    = (int16_t)(t * SCALE_TEMP_X10);
    pressurePa   = (uint32_t)p;
    altitudeM_x10 = (int16_t)(alt * SCALE_ALTITUDE_X10);
    return true;
}
