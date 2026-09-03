#include "soil_telemetry.h"
#include "dht_sensor.h"
#include "rain_sensor.h"
#include "soil_moisture.h"
#include "light_sensor.h"

void soil_telemetry_init() {
    dht_init();
    light_init();
    // rain/soil are analog-only; no pin setup required beyond analogRead.
}

void soil_telemetry_collect(SoilPacket& outPacket) {
    outPacket.packetId = PACKET_SOIL;
    outPacket.statusFlags = 0;

    int16_t tempC_x10;
    uint16_t humidity_x10;
    if (dht_read(tempC_x10, humidity_x10)) {
        outPacket.dhtTempC_x10 = tempC_x10;
        outPacket.dhtHumidity_x10 = humidity_x10;
        outPacket.statusFlags |= SOIL_STATUS_DHT_VALID;
    } else {
        outPacket.dhtTempC_x10 = 0;
        outPacket.dhtHumidity_x10 = 0;
    }

    // Analog sensors are always physically readable (a disconnected
    // sensor reads noise, not an error), so they are always marked valid.
    rain_read(outPacket.rainRaw, outPacket.rainState);
    outPacket.statusFlags |= SOIL_STATUS_RAIN_VALID;

    soil_read(outPacket.soilRaw, outPacket.soilPct, outPacket.soilState);
    outPacket.statusFlags |= SOIL_STATUS_SOIL_VALID;

    outPacket.lightState = light_read();
    outPacket.statusFlags |= SOIL_STATUS_LIGHT_VALID;
}
