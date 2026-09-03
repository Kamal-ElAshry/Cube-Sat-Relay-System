// Station_A_Soil.ino
//
// Arduino Nano + DHT11 + rain sensor + soil moisture sensor + LDR + nRF24L01.
// Reads all four sensors on a fixed interval, transmits a SoilPacket to the
// CubeSat(s) on a separate interval, per the RF mode configured in
// radio_config.h. No delay(), no blocking waits - a failed sensor or a
// failed radio never stops the rest of the system.

#include "config.h"
#include "radio_config.h"
#include "packet_types.h"
#include "radio.h"
#include "soil_telemetry.h"
#include "dht_sensor.h"
#include "rain_sensor.h"
#include "soil_moisture.h"
#include "status_output.h"

static unsigned long _lastSensorMs = 0;
static unsigned long _lastSendMs   = 0;
static unsigned long _lastStatusMs = 0;
static SoilPacket    _latestPacket;

void setup() {
    status_output_init(); // Serial.begin(SERIAL_BAUD) for on-bench debugging
    soil_telemetry_init();
    radio_init(); // failure is non-fatal; radio_isAvailable() reflects status
    soil_telemetry_collect(_latestPacket);
}

void loop() {
    unsigned long now = millis();

    radio_maintain();

    if (now - _lastSensorMs >= SENSOR_INTERVAL_MS) {
        _lastSensorMs = now;
        soil_telemetry_collect(_latestPacket);
    }

    if (now - _lastSendMs >= RF_SEND_INTERVAL_MS) {
        _lastSendMs = now;
        if (radio_isAvailable()) {
            radio_send((const uint8_t*)&_latestPacket, sizeof(_latestPacket));
        }
        // Radio down: simply skip this cycle. Sensors keep running and the
        // next cycle will try again; radio_maintain() retries init in the
        // background.
    }

    if (now - _lastStatusMs >= STATUS_PRINT_INTERVAL_MS) {
        _lastStatusMs = now;
               status_output_print(
            _latestPacket,
            rain_isWorking(),
            soil_isWorking(),
            radio_isAvailable(),
            radio_getLastSendSuccess()
        );
    }
}
