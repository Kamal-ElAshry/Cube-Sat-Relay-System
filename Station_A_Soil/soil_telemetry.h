#pragma once
#include "packet_types.h"

void soil_telemetry_init();

// Collects all four sensors into one SoilPacket, setting statusFlags
// per-sensor. A failed sensor yields a deterministic zero value for
// its fields with its validity bit cleared - the rest of the packet
// is unaffected (section 23).
void soil_telemetry_collect(SoilPacket& outPacket);
