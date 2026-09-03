#pragma once
#include "packet_types.h"

void telemetry_init();

// --- Soil cache (received via RF from Station A) ---
void telemetry_updateSoilCache(const SoilPacket& pkt);
bool telemetry_hasSoilData();
const SoilPacket& telemetry_getSoilCache();

// --- Cube telemetry (this CubeSat's own sensors) ---
// Failed sensors yield deterministic zero fields with the corresponding
// validity bit cleared; the rest of the packet is still built and sent.
void telemetry_collectCube(CubePacket& outPacket);
