#include "telemetry.h"
#include "imu.h"
#include "barometer.h"

static SoilPacket _soilCache;
static bool       _hasSoilData = false;

void telemetry_init() {
    imu_init();
    barometer_init();
    _hasSoilData = false;
}

void telemetry_updateSoilCache(const SoilPacket& pkt) {
    _soilCache = pkt;
    _hasSoilData = true;
}

bool telemetry_hasSoilData() {
    return _hasSoilData;
}

const SoilPacket& telemetry_getSoilCache() {
    return _soilCache;
}

void telemetry_collectCube(CubePacket& outPacket) {
    outPacket.packetId = PACKET_CUBE;
    outPacket.statusFlags = 0;

    int16_t ax, ay, az, gx, gy, gz;
    if (imu_read(ax, ay, az, gx, gy, gz)) {
        outPacket.accelX = ax; outPacket.accelY = ay; outPacket.accelZ = az;
        outPacket.gyroX  = gx; outPacket.gyroY  = gy; outPacket.gyroZ  = gz;
        outPacket.statusFlags |= CUBE_STATUS_MPU_VALID;
    } else {
        outPacket.accelX = 0; outPacket.accelY = 0; outPacket.accelZ = 0;
        outPacket.gyroX  = 0; outPacket.gyroY  = 0; outPacket.gyroZ  = 0;
    }

    int16_t  t;
    uint32_t p;
    int16_t  a;
    if (barometer_read(t, p, a)) {
        outPacket.bmpTempC_x10 = t;
        outPacket.bmpPressurePa = p;
        outPacket.bmpAltitudeM_x10 = a;
        outPacket.statusFlags |= CUBE_STATUS_BMP_VALID;
    } else {
        outPacket.bmpTempC_x10 = 0;
        outPacket.bmpPressurePa = 0;
        outPacket.bmpAltitudeM_x10 = 0;
    }
}
