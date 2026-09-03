#pragma once
//
// radio_config.h - CubeSat RF link configuration.
//
// The CubeSat sits on TWO independent links using the SAME physical
// radio:
//   LINK A (Soil -> Cube): this file is the RECEIVING side - must
//                            mirror Station_A_Soil/radio_config.h.
//   LINK B (Cube -> Station B): this file is the TRANSMITTING side -
//                            configured independently of Link A.
//
// Changing one link's mode never requires changing the other's.
//
#include <RF24.h>
#include "packet_types.h"

// ---------------------------------------------------------------------
// Common radio parameters (shared by both links - one physical radio)
// ---------------------------------------------------------------------
constexpr uint8_t          RF_CHANNEL       = 76;
constexpr rf24_datarate_e  RF_DATA_RATE     = RF24_1MBPS;
constexpr rf24_pa_dbm_e    RF_PA_LEVEL      = RF24_PA_LOW;
constexpr uint8_t          RF_ADDRESS_WIDTH = 5;
constexpr uint8_t          RF_RETRY_DELAY   = 15;
constexpr uint8_t          RF_RETRY_COUNT   = 15;

// =======================================================================
// LINK A: Soil -> Cube (RECEIVE side)
// =======================================================================
// Must match Station A's SOIL_TO_CUBE_MODE exactly.
constexpr RfDistributionMode SOIL_TO_CUBE_MODE = RfDistributionMode::EXPLICIT_MULTI_RECEIVER;

// Used when SOIL_TO_CUBE_MODE == SHARED_MULTI_RECEIVER.
// Must match Station A's SHARED_ADDRESS_A_TO_CUBE.
constexpr uint8_t SHARED_ADDRESS_A_TO_CUBE[RF_ADDRESS_WIDTH] = {'S', 'O', 'I', 'L', '1'};

// Used when SOIL_TO_CUBE_MODE == ONE_TO_ONE or EXPLICIT_MULTI_RECEIVER.
// THIS SPECIFIC CUBESAT's own receive address - must match exactly one
// entry in Station A's CUBESAT_ADDRESSES[] table. Each physical CubeSat
// unit gets its own value here.
constexpr uint8_t MY_ADDRESS_FROM_A[RF_ADDRESS_WIDTH] = {'C', 'U', 'B', 'E', '1'};

// =======================================================================
// LINK B: Cube -> Station B (TRANSMIT side)
// =======================================================================
// Independently configurable from LINK A above - CHANGE ONLY THIS LINE
// TO RECONFIGURE HOW THIS CUBESAT DISTRIBUTES ITS TELEMETRY.
constexpr RfDistributionMode CUBE_TO_STATIONB_MODE = RfDistributionMode::ONE_TO_ONE;

// Used when CUBE_TO_STATIONB_MODE == SHARED_MULTI_RECEIVER.
constexpr uint8_t SHARED_ADDRESS_CUBE_TO_B[RF_ADDRESS_WIDTH] = {'C', 'U', 'B', 'E', 'B'};

// Used when CUBE_TO_STATIONB_MODE == ONE_TO_ONE or EXPLICIT_MULTI_RECEIVER.
// ONE_TO_ONE uses only STATIONB_ADDRESSES[0]; EXPLICIT_MULTI_RECEIVER
// cycles through all STATIONB_ADDRESS_COUNT entries.
constexpr uint8_t STATIONB_ADDRESS_COUNT = 2;
constexpr uint8_t STATIONB_ADDRESSES[STATIONB_ADDRESS_COUNT][RF_ADDRESS_WIDTH] = {
    {'S', 'T', 'B', '0', '1'},
    {'S', 'T', 'B', '0', '2'},
};
