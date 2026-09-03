#include "rain_sensor.h"
#include "config.h"
#include "packet_types.h"

#include <Arduino.h>

// 1024 is outside the valid 0-1023 ADC range, so it safely means
// "no sample taken yet" and can never falsely match a real reading.
static uint16_t _lastRaw = 1024;
static uint16_t _stuckCount = 0;

void rain_read(uint16_t& outRaw, uint8_t& outState) {
    outRaw = analogRead(PIN_RAIN_AO);

    if (outRaw == _lastRaw) {
        if (_stuckCount < 0xFFFF) _stuckCount++;
    } else {
        _lastRaw = outRaw;
        _stuckCount = 0;
    }

    bool wet = RAIN_TRIGGERS_BELOW_THRESHOLD
                   ? (outRaw < RAIN_ADC_THRESHOLD)
                   : (outRaw > RAIN_ADC_THRESHOLD);

    outState = wet ? RAIN_DETECTED : RAIN_NONE;
}

bool rain_isWorking() {
    return _stuckCount < STUCK_SAMPLE_THRESHOLD;
}
