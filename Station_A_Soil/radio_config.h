#pragma once
//
// radio_config.h - Station A RF link configuration (LINK A: Soil -> CubeSat)
//
// To change how Station A distributes packets to CubeSat(s), change ONLY
// the SOIL_TO_CUBE_MODE line below. Nothing else in this project needs
// to change - radio.cpp branches on this single value.
//
// IMPORTANT: whichever mode you pick here MUST be mirrored in
// CubeSat/radio_config.h's SOIL_TO_CUBE_MODE, since that's the receiving
// side of this same link.
//
#include <RF24.h>
#include "packet_types.h"

// ---------------------------------------------------------------------
// LINK A mode selection - CHANGE ONLY THIS LINE TO RECONFIGURE
// ---------------------------------------------------------------------
constexpr RfDistributionMode SOIL_TO_CUBE_MODE = RfDistributionMode::EXPLICIT_MULTI_RECEIVER;

// ---------------------------------------------------------------------
// Common radio parameters
// ---------------------------------------------------------------------
constexpr uint8_t          RF_CHANNEL      = 76;
constexpr rf24_datarate_e  RF_DATA_RATE    = RF24_1MBPS;
constexpr rf24_pa_dbm_e    RF_PA_LEVEL     = RF24_PA_LOW; // see wiring notes re: 3.3V supply current
constexpr uint8_t          RF_ADDRESS_WIDTH = 5;
constexpr uint8_t          RF_RETRY_DELAY  = 15; // * 250us, only relevant to ACK-based writes
constexpr uint8_t          RF_RETRY_COUNT  = 15;

// ---------------------------------------------------------------------
// Used only when SOIL_TO_CUBE_MODE == SHARED_MULTI_RECEIVER
// ---------------------------------------------------------------------
constexpr uint8_t SHARED_ADDRESS_A_TO_CUBE[RF_ADDRESS_WIDTH] = {'S', 'O', 'I', 'L', '1'};

// ---------------------------------------------------------------------
// Used when SOIL_TO_CUBE_MODE == ONE_TO_ONE or EXPLICIT_MULTI_RECEIVER
// ---------------------------------------------------------------------
// ONE_TO_ONE uses only CUBESAT_ADDRESSES[0]; EXPLICIT_MULTI_RECEIVER
// cycles through all CUBESAT_ADDRESS_COUNT entries. You can leave extra
// entries in the table even in ONE_TO_ONE mode - they're simply unused.
//
// Each entry must match the MY_ADDRESS_FROM_A constant configured into
// the corresponding physical CubeSat's own radio_config.h.
constexpr uint8_t CUBESAT_ADDRESS_COUNT = 2;
constexpr uint8_t CUBESAT_ADDRESSES[CUBESAT_ADDRESS_COUNT][RF_ADDRESS_WIDTH] = {
    {'C', 'U', 'B', 'E', '1'},
    {'C', 'U', 'B', 'E', '2'},
};
