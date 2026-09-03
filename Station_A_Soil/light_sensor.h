#pragma once
#include <stdint.h>

// Digital-only LDR module: reports whether light is detected relative
// to the module's onboard potentiometer threshold. It cannot determine
// lux, and it cannot determine the Sun's compass direction - only
// whether light is currently hitting the sensor.
void light_init();
uint8_t light_read(); // returns a LightState value (see packet_types.h)
