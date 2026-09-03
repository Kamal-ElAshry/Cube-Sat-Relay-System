#pragma once
#include <stdint.h>

bool dht_init();
bool dht_isAvailable();

// Returns true and fills outputs on success. Returns false (and leaves
// outputs untouched) if the sensor read failed - caller is responsible
// for supplying a deterministic fallback.
bool dht_read(int16_t& outTempC_x10, uint16_t& outHumidity_x10);
