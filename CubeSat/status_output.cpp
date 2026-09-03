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

void status_output_print(const CubePacket& pkt, bool radioAvailable,
                          bool linkAReceived, bool linkBSent) {
    bool mpuWorking = (pkt.statusFlags & CUBE_STATUS_MPU_VALID) != 0;
    bool bmpWorking = (pkt.statusFlags & CUBE_STATUS_BMP_VALID) != 0;

    Serial.print(F("[STATUS] "));

    _printFlagLabel(F(" | Mpu="), mpuWorking);
    if (mpuWorking) {
        Serial.print(F(" (AX= "));
        Serial.print(pkt.accelX);
        Serial.print(F(" AY= "));
        Serial.print(pkt.accelY);
        Serial.print(F(" AZ= "));
        Serial.print(pkt.accelZ);
        Serial.print(F(" GX= "));
        Serial.print(pkt.gyroX);
        Serial.print(F(" GY= "));
        Serial.print(pkt.gyroY);
        Serial.print(F(" GZ= "));
        Serial.print(pkt.gyroZ);
        Serial.print(F(")"));
    }
    Serial.print(' ');

    _printFlagLabel(F(" | Bmp="), bmpWorking);
    if (bmpWorking) {
        Serial.print(F(" ("));
        _printFixed1(pkt.bmpTempC_x10);
        Serial.print(F("C "));
        Serial.print(pkt.bmpPressurePa);
        Serial.print(F("Pa "));
        _printFixed1(pkt.bmpAltitudeM_x10);
        Serial.print(F("m)"));
    }
    Serial.print(' ');

    _printFlagLabel(F(" | RF="), radioAvailable);
    Serial.print(' ');

    Serial.print(F(" | LinkA_Mode="));
    Serial.print(_modeName(SOIL_TO_CUBE_MODE));
    Serial.print(F(" | LinkA_Received="));
    Serial.print(linkAReceived ? F("Yes") : F("No"));

    Serial.print(F(" | LinkB_Mode="));
    Serial.print(_modeName(CUBE_TO_STATIONB_MODE));
    Serial.print(F(" | LinkB_Sent="));
    Serial.println(linkBSent ? F("Yes") : F("No"));
}