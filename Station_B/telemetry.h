#pragma once
#include "packet_types.h"

void telemetry_init();

void telemetry_updateSoil(const SoilPacket& pkt);
void telemetry_updateCube(const CubePacket& pkt);

bool telemetry_hasSoilData();
bool telemetry_hasCubeData();

const SoilPacket& telemetry_getSoil();
const CubePacket& telemetry_getCube();
