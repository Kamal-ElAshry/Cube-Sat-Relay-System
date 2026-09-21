#pragma once
#include <stdint.h>

bool ina219_init();
bool ina219_isAvailable();
bool ina219_read(int32_t& current_mA, uint16_t& busVoltage_mV, uint32_t& power_mW);

// Call every loop(). Non-blocking: if the INA219 failed to init, retries
// every INA219_RETRY_INTERVAL_MS in the background. Does nothing while
// the sensor is already available. Same pattern as barometer_maintain().
void ina219_maintain();
