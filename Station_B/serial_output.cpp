#include "serial_output.h"
#include "config.h"

#include <Arduino.h>

// Prints a value scaled by 10 as a fixed-point 1-decimal number,
// e.g. -37 -> "-3.7", 0 -> "0.0". Used consistently (including at the
// zero/never-received state) so the output format never changes shape.
static void _printFixed1(int16_t valueX10) {
    if (valueX10 < 0) {
        Serial.print('-');
        valueX10 = -valueX10;
    }
    Serial.print(valueX10 / 10);
    Serial.print('.');
    Serial.print(valueX10 % 10);
}

void serial_output_init() {
    Serial.begin(SERIAL_BAUD);
}

void serial_output_print(const SoilPacket& soil, const CubePacket& cube, bool radioAvailable) {
    if (!radioAvailable) {
        // The nRF24 chip itself never initialized - distinct from "radio is
        // fine, nothing received yet", which still prints the normal
        // telemetry line (with zero/last-valid fields) below.
        Serial.println(F("RF: Not Connected"));
        return;
    }

    Serial.print(F("Soil: Temp="));
    _printFixed1(soil.dhtTempC_x10);
    Serial.print(F("C ; Hum="));
    _printFixed1((int16_t)soil.dhtHumidity_x10);
    Serial.print(F("% ; RainRaw="));
    Serial.print(soil.rainRaw);
    Serial.print(F(" ; Rain="));
    Serial.print(soil.rainState == RAIN_DETECTED ? F("Detected") : F("None"));
    Serial.print(F(" ; SoilRaw="));
    Serial.print(soil.soilRaw);
    Serial.print(F(" ; SoilPct="));
    Serial.print(soil.soilPct);
    Serial.print(F("% ; Soil="));
    switch (soil.soilState) {
        case SOIL_DRY:         Serial.print(F("Dry"));         break;
        case SOIL_OVERWATERED: Serial.print(F("Overwatered")); break;
        default:                Serial.print(F("Normal"));      break;
    }
    Serial.print(F(" Light="));
    Serial.print(soil.lightState == LIGHT_DETECTED ? F("Detected") : F("Dark"));

    Serial.print(F(" | Cube: AX="));
    Serial.print(cube.accelX);
    Serial.print(F(" ; AY="));
    Serial.print(cube.accelY);
    Serial.print(F(" ; AZ="));
    Serial.print(cube.accelZ);
    Serial.print(F(" ; GX="));
    Serial.print(cube.gyroX);
    Serial.print(F(" ; GY="));
    Serial.print(cube.gyroY);
    Serial.print(F(" ; GZ="));
    Serial.print(cube.gyroZ);
    Serial.print(F(" ; Temp="));
    _printFixed1(cube.bmpTempC_x10);
    Serial.print(F("C ; Pressure="));
    Serial.print(cube.bmpPressurePa);
    Serial.print(F("Pa ; Altitude="));
    _printFixed1(cube.bmpAltitudeM_x10);
    Serial.println(F("m"));
}
