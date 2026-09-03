#pragma once
#include <stdint.h>

// Analog sensor - always physically readable, so there is no
// init()/isAvailable() pair. A disconnected sensor just reads noise,
// which is an inherent property of analog inputs, not a software fault.
void rain_read(uint16_t& outRaw, uint8_t& outState);

// Debug-only heuristic: false if the raw reading has stayed exactly
// frozen for STUCK_SAMPLE_THRESHOLD consecutive calls to rain_read().
// Does not affect statusFlags in the transmitted SoilPacket.
bool rain_isWorking();
