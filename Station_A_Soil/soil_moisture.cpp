#include "soil_moisture.h"
#include "config.h"
#include "packet_types.h"

#include <Arduino.h>

// 1024 is outside the valid 0-1023 ADC range, so it safely means
// "no sample taken yet" and can never falsely match a real reading.
static uint16_t _lastRaw = 1024;
static uint16_t _stuckCount = 0;

void soil_read(uint16_t& outRaw, uint8_t& outPct, uint8_t& outState) {
    outRaw = analogRead(PIN_SOIL_AO);

    if (outRaw == _lastRaw) {
        if (_stuckCount < 0xFFFF) _stuckCount++;
    } else {
        _lastRaw = outRaw;
        _stuckCount = 0;
    }

    // map() interpolates correctly regardless of whether SOIL_ADC_DRY is
    // greater or less than SOIL_ADC_WET, so reversed sensor polarity is
    // handled simply by calibrating these two constants correctly in
    // config.h - no separate branch needed here.
    long pct = map((long)outRaw, (long)SOIL_ADC_DRY, (long)SOIL_ADC_WET, 0, 100);
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    outPct = (uint8_t)pct;

    if (outPct <= SOIL_PCT_DRY_THRESHOLD) {
        outState = SOIL_DRY;
    } else if (outPct >= SOIL_PCT_OVERWATERED_THRESHOLD) {
        outState = SOIL_OVERWATERED;
    } else {
        outState = SOIL_NORMAL;
    }
}

bool soil_isWorking() {
    return _stuckCount < STUCK_SAMPLE_THRESHOLD;
}
