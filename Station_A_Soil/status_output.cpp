#include "status_output.h"
#include "config.h"
#include "radio_config.h"

#include <Arduino.h>

// ---------------------------------------------------------------------
// All wording for the status line lives in this file, using the F()
// macro so the literals stay in flash instead of consuming RAM. Edit the
// text inside the F("...") calls below to relabel the output.
// ---------------------------------------------------------------------

// Prints a value scaled by 10 as a fixed-point 1-decimal number,
// e.g. -37 -> "-3.7", 0 -> "0.0".
static void _printFixed1(int16_t valueX10) {
    if (valueX10 < 0) {
        Serial.print('-');
        valueX10 = -valueX10;
    }
    Serial.print(valueX10 / 10);
    Serial.print('.');
    Serial.print(valueX10 % 10);
}

static void _printFlagLabel(const __FlashStringHelper* label, bool working) {
    Serial.print(label);
    Serial.print(working ? F("Working") : F("Not Working"));
}

static const __FlashStringHelper* _modeName(RfDistributionMode mode) {
    switch (mode) {
        case RfDistributionMode::ONE_TO_ONE:              return F("ONE_TO_ONE");
        case RfDistributionMode::SHARED_MULTI_RECEIVER:   return F("SHARED_MULTI_RECEIVER");
        case RfDistributionMode::EXPLICIT_MULTI_RECEIVER: return F("EXPLICIT_MULTI_RECEIVER");
        default:                                          return F("UNKNOWN");
    }
}

void status_output_init() {
    Serial.begin(SERIAL_BAUD);
}

void status_output_print(const SoilPacket& pkt, bool rainWorking, bool soilWorking,
                          bool radioAvailable, bool lastSendSuccess) {
    bool dhtWorking   = (pkt.statusFlags & SOIL_STATUS_DHT_VALID)   != 0;
    bool lightWorking = (pkt.statusFlags & SOIL_STATUS_LIGHT_VALID) != 0;

    Serial.print(F("[STATUS] "));

    _printFlagLabel(F(" | Dht="), dhtWorking);
    if (dhtWorking) {
        Serial.print(F(" ("));
        _printFixed1(pkt.dhtTempC_x10);
        Serial.print(F("C "));
        _printFixed1((int16_t)pkt.dhtHumidity_x10);
        Serial.print(F("%)"));
    }
    Serial.print(' ');

    _printFlagLabel(F(" | Rain="), rainWorking);
    if (rainWorking) {
        Serial.print(F(" ("));
        Serial.print(pkt.rainRaw);
        Serial.print(' ');
        Serial.print(pkt.rainState == RAIN_DETECTED ? F("Detected") : F("None"));
        Serial.print(F(")"));
    }
    Serial.print(' ');

    _printFlagLabel(F(" | Soil="), soilWorking);
    if (soilWorking) {
        Serial.print(F(" ("));
        Serial.print(pkt.soilRaw);
        Serial.print(' ');
        Serial.print(pkt.soilPct);
        Serial.print(F("% "));
        switch (pkt.soilState) {
            case SOIL_DRY:         Serial.print(F("Dry"));         break;
            case SOIL_OVERWATERED: Serial.print(F("Overwatered")); break;
            default:                Serial.print(F("Normal"));      break;
        }
        Serial.print(F(")"));
    }
    Serial.print(' ');

    _printFlagLabel(F(" | Light="), lightWorking);
    if (lightWorking) {
        Serial.print(F(" ("));
        Serial.print(pkt.lightState == LIGHT_DETECTED ? F("Detected") : F("Dark"));
        Serial.print(F(")"));
    }
    Serial.print(' ');

    _printFlagLabel(F("RF="), radioAvailable);
    Serial.print(' ');

    Serial.print(F("| Mode="));
    Serial.print(_modeName(SOIL_TO_CUBE_MODE));
    Serial.print(F(" | Sent="));
    Serial.println(lastSendSuccess ? F("Yes") : F("No"));
}