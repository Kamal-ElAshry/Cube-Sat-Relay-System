#include "light_sensor.h"
#include "config.h"
#include "packet_types.h"

#include <Arduino.h>

void light_init() {
    pinMode(PIN_LIGHT_DO, INPUT);
}

uint8_t light_read() {
    int level = digitalRead(PIN_LIGHT_DO);
    bool detected = (level == LDR_ACTIVE_LEVEL);
    return detected ? (uint8_t)LIGHT_DETECTED : (uint8_t)LIGHT_DARK;
}
