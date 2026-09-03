#pragma once
#include <stdint.h>

bool radio_init();
bool radio_isAvailable();
void radio_maintain(); // call every loop(); non-blocking re-init retry if radio is down

// Non-blocking. Returns true and fills outBuffer (exactly RF_PAYLOAD_SIZE
// bytes) if a new raw packet arrived. Dispatch on outBuffer[0].
bool radio_pollPacket(uint8_t* outBuffer, uint8_t& outLen);
