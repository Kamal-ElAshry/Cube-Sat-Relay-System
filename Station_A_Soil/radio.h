#pragma once
//
// radio.h - RF transport abstraction. The application layer never sees
// addresses, ACK configuration, or distribution-mode branching - it just
// calls radio_send() and gets back a result.
//
#include <stdint.h>

struct SendOutcome {
    bool    anyTransmitAttempted; // false only if radio was unavailable
    bool    anySuccess;           // SHARED mode: mirrors "radio finished the TX", NOT receiver confirmation
    uint8_t successCount;         // ADDRESSED-style modes: # destinations ACKed this cycle
    uint8_t failureCount;         // ADDRESSED-style modes: # destinations that failed to ACK this cycle
};

// Per-destination result, only meaningful when SOIL_TO_CUBE_MODE is
// ONE_TO_ONE or EXPLICIT_MULTI_RECEIVER. Exposed for future
// diagnostics (e.g. a status LED) - not currently required by any
// consumer in this project.
struct ReceiverStatus {
    bool          lastSuccess;
    uint16_t      failureCount;
    unsigned long lastAttemptMs;
};

bool radio_init();
bool radio_isAvailable();
void radio_maintain(); // call every loop(); non-blocking re-init retry if radio is down

// Sends one logical packet according to the configured SOIL_TO_CUBE_MODE.
// Internally zero-pads to RF_PAYLOAD_SIZE before transmitting.
SendOutcome radio_send(const uint8_t* payload, uint8_t len);

// True if the most recent radio_send() call reported anySuccess. Debug
// convenience only - equivalent to checking the SendOutcome yourself.
bool radio_getLastSendSuccess();

// Returns nullptr if index is out of range, or if the current mode is
// SHARED_MULTI_RECEIVER (no per-destination tracking exists in that mode).
const ReceiverStatus* radio_getDestinationStatus(uint8_t index);
uint8_t radio_getDestinationCount();
