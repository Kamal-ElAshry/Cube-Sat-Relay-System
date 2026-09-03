#include "telemetry.h"
#include <string.h>

static SoilPacket _soilCache;
static CubePacket _cubeCache;
static bool       _hasSoil = false;
static bool       _hasCube = false;

void telemetry_init() {
    memset(&_soilCache, 0, sizeof(_soilCache));
    memset(&_cubeCache, 0, sizeof(_cubeCache));
    _soilCache.packetId = PACKET_SOIL;
    _cubeCache.packetId = PACKET_CUBE;
    _hasSoil = false;
    _hasCube = false;
}

void telemetry_updateSoil(const SoilPacket& pkt) {
    _soilCache = pkt;
    _hasSoil = true;
}

void telemetry_updateCube(const CubePacket& pkt) {
    _cubeCache = pkt;
    _hasCube = true;
}

bool telemetry_hasSoilData() { return _hasSoil; }
bool telemetry_hasCubeData() { return _hasCube; }

const SoilPacket& telemetry_getSoil() { return _soilCache; }
const CubePacket& telemetry_getCube() { return _cubeCache; }
