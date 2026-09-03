#pragma once
//
// radio.h - CubeSat RF transport abstraction. Single physical radio,
// time-multiplexed between listening (Link A) and transmitting (Link B).
// The application layer never touches addresses, ACK config, or the
// RX/TX switching mechanics directly.
//
#include <stdint.h>
#include "packet_types.h"

struct SendOutcome {
    bool    anyTransmitAttempted;
    bool    anySuccess;
    uint8_t successCount;
    uint8_t failureCount;
};

struct ReceiverStatus {
    bool          lastSuccess;
    uint16_t      failureCount;
    unsigned long lastAttemptMs;
};

bool radio_init();
bool radio_isAvailable();
void radio_maintain(); // call every loop(); non-blocking re-init retry if radio is down

// Non-blocking. Call every loop() while "listening" (the default state).
// Returns true and fills outPacket if a new, valid Soil packet arrived.
bool radio_pollSoil(SoilPacket& outPacket);

// Switches to TX, sends soilToRelay (if non-null) then cubePacket
// according to CUBE_TO_STATIONB_MODE, then switches back to listening.
// Pass nullptr for soilToRelay if no valid Soil packet has ever been
// received yet - the CubeSat still sends its own CubePacket regardless.
SendOutcome radio_relayAndSend(const SoilPacket* soilToRelay, const CubePacket& cubePacket);

// Only meaningful when CUBE_TO_STATIONB_MODE != SHARED_MULTI_RECEIVER.
const ReceiverStatus* radio_getStationBStatus(uint8_t index);
uint8_t radio_getStationBCount();

// True if the most recent radio_relayAndSend() call reported anySuccess
// (across whichever packets/destinations it attempted that cycle).
// Debug convenience only.
bool radio_getLastSendSuccess();
