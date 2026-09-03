// CubeSat.ino
//
// Arduino UNO + MPU6050 + BMP180 + nRF24L01 (single radio, time-multiplexed).
// Continuously listens for Soil packets from Station A and caches the
// latest valid one; independently reads its own IMU/baro on a fixed
// interval; periodically switches briefly to TX to relay the cached Soil
// packet plus its own CubePacket, then returns to listening.
//
// No mission counter. No delay(). A failed sensor, a failed Soil link, or
// a failed Station B link never freezes the CubeSat or destroys valid
// cached data from the other source.

#include "config.h"
#include "radio_config.h"
#include "packet_types.h"
#include "radio.h"
#include "telemetry.h"
#include "imu.h"
#include "barometer.h"
#include "status_output.h"

static unsigned long _lastSensorMs = 0;
static unsigned long _lastRelayMs  = 0;
static unsigned long _lastStatusMs = 0;
static CubePacket    _latestCube;

void setup() {
    status_output_init(); // Serial.begin(SERIAL_BAUD) for on-bench debugging
    telemetry_init();
    radio_init(); // failure is non-fatal; radio_isAvailable() reflects status
    telemetry_collectCube(_latestCube);
}

void loop() {
    unsigned long now = millis();

    radio_maintain();
    barometer_maintain(); // non-blocking background retry if BMP180 init failed

    // Continuously poll for an incoming Soil packet while in the default
    // listening state. This never blocks - available()/read() return
    // immediately either way.
    SoilPacket incomingSoil;
    if (radio_pollSoil(incomingSoil)) {
        telemetry_updateSoilCache(incomingSoil);
    }

    if (now - _lastSensorMs >= SENSOR_INTERVAL_MS) {
        _lastSensorMs = now;
        telemetry_collectCube(_latestCube);
    }

    if (now - _lastRelayMs >= RELAY_TX_INTERVAL_MS) {
        _lastRelayMs = now;
        if (radio_isAvailable()) {
            // Only relay Soil data once we've actually received a valid
            // packet at least once - we never fabricate Soil telemetry.
            // The CubeSat's own telemetry is always sent regardless.
            const SoilPacket* relay = telemetry_hasSoilData() ? &telemetry_getSoilCache() : nullptr;
            radio_relayAndSend(relay, _latestCube);
        }
        // Radio down: skip this cycle; RX polling and sensor reads above
        // are entirely unaffected, and radio_maintain() retries in the
        // background.
    }

    if (now - _lastStatusMs >= STATUS_PRINT_INTERVAL_MS) {
        _lastStatusMs = now;
               status_output_print(
            _latestCube,
            radio_isAvailable(),
            telemetry_hasSoilData(),
            radio_getLastSendSuccess()
        );
    }
}
