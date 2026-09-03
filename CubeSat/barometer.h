#pragma once
#include <stdint.h>

bool barometer_init();
bool barometer_isAvailable();
bool barometer_read(int16_t& tempC_x10, uint32_t& pressurePa, int16_t& altitudeM_x10);

// Call every loop(). Non-blocking: if the BMP180 failed to init, retries
// every BAROMETER_RETRY_INTERVAL_MS in the background. Does nothing while
// the sensor is already available.
void barometer_maintain();
