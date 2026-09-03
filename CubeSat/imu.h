#pragma once
#include <stdint.h>

// Raw register access only - no filtering, no pitch/roll/orientation
// calculation, no sensor fusion. Per spec, derived values are explicitly
// out of scope for now.
bool imu_init();
bool imu_isAvailable();
bool imu_read(int16_t& ax, int16_t& ay, int16_t& az,
               int16_t& gx, int16_t& gy, int16_t& gz);
