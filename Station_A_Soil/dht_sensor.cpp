#include "dht_sensor.h"
#include "config.h"

#include <DHT.h>
#include <math.h>

static DHT  _dht(PIN_DHT11, DHT11);
static bool _available = false;

bool dht_init() {
    _dht.begin();
    // DHT11 has no explicit "begin succeeded" signal from the library;
    // probe with one read (per section 22: attempt once, don't block).
    float t = _dht.readTemperature();
    _available = !isnan(t);
    return _available;
}

bool dht_isAvailable() {
    return _available;
}

bool dht_read(int16_t& outTempC_x10, uint16_t& outHumidity_x10) {
    float t = _dht.readTemperature();
    float h = _dht.readHumidity();

    if (isnan(t) || isnan(h)) {
        _available = false;
        return false;
    }

    _available = true;
    outTempC_x10 = (int16_t)(t * SCALE_TEMP_X10);
    outHumidity_x10 = (uint16_t)(h * SCALE_HUMIDITY_X10);
    return true;
}
