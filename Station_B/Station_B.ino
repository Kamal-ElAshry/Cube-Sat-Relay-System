// Station_B.ino
//
// Arduino Nano + nRF24L01 + USB Serial (115200, LabVIEW-facing).
// Pure receiver: listens continuously, dispatches on packet ID, maintains
// independent last-valid-value caches for Soil and Cube telemetry, and
// prints exactly ONE deterministic serial line per interval. No debug
// text is ever written to Serial - see serial_output.cpp.

#include "config.h"
#include "radio_config.h"
#include "packet_types.h"
#include "radio.h"
#include "telemetry.h"
#include "serial_output.h"

#include <string.h>

static unsigned long _lastPrintMs = 0;

void setup() {
    serial_output_init();
    telemetry_init();
    radio_init(); // failure is non-fatal; radio_maintain() retries in the background
}

void loop() {
    radio_maintain();

    uint8_t buffer[32];
    uint8_t len = 0;
    if (radio_pollPacket(buffer, len)) {
        uint8_t packetId = buffer[0];
        switch (packetId) {
            case PACKET_SOIL: {
                SoilPacket pkt;
                memcpy(&pkt, buffer, sizeof(SoilPacket));
                telemetry_updateSoil(pkt);
                break;
            }
            case PACKET_CUBE: {
                CubePacket pkt;
                memcpy(&pkt, buffer, sizeof(CubePacket));
                telemetry_updateCube(pkt);
                break;
            }
            default:
                // Unknown packet: ignored safely. No error is printed to
                // the LabVIEW-facing serial stream.
                break;
        }
    }

    unsigned long now = millis();
    if (now - _lastPrintMs >= PRINT_INTERVAL_MS) {
        _lastPrintMs = now;
        serial_output_print(telemetry_getSoil(), telemetry_getCube(), radio_isAvailable());
    }
}
