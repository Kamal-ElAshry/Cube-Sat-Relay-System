#pragma once
#include <stdint.h>

void soil_read(uint16_t& outRaw, uint8_t& outPct, uint8_t& outState);

// Debug-only heuristic: false if the raw reading has stayed exactly
// frozen for STUCK_SAMPLE_THRESHOLD consecutive calls to soil_read().
// Does not affect statusFlags in the transmitted SoilPacket.
bool soil_isWorking();
