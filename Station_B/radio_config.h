#pragma once
//
// radio_config.h - Station B RF link configuration (LINK B: Cube -> Station B)
//
// This is the RECEIVING side of Link B. Every value here must mirror the
// corresponding constant in CubeSat/radio_config.h's "LINK B" section.
//
#include <RF24.h>
#include "packet_types.h"

// Must match CubeSat's CUBE_TO_STATIONB_MODE exactly.
constexpr RfDistributionMode CUBE_TO_STATIONB_MODE = RfDistributionMode::ONE_TO_ONE;

constexpr uint8_t          RF_CHANNEL       = 76;
constexpr rf24_datarate_e  RF_DATA_RATE     = RF24_1MBPS;
constexpr rf24_pa_dbm_e    RF_PA_LEVEL      = RF24_PA_LOW;
constexpr uint8_t          RF_ADDRESS_WIDTH = 5;

// Used when CUBE_TO_STATIONB_MODE == SHARED_MULTI_RECEIVER.
// Must match CubeSat's SHARED_ADDRESS_CUBE_TO_B.
constexpr uint8_t SHARED_ADDRESS_CUBE_TO_B[RF_ADDRESS_WIDTH] = {'C', 'U', 'B', 'E', 'B'};

// Used when CUBE_TO_STATIONB_MODE == ONE_TO_ONE or EXPLICIT_MULTI_RECEIVER.
// THIS SPECIFIC Station B's own receive address - must match exactly one
// entry in the CubeSat's STATIONB_ADDRESSES[] table. Each physical
// Station B unit gets its own value here.
constexpr uint8_t MY_ADDRESS_FROM_CUBE[RF_ADDRESS_WIDTH] = {'S', 'T', 'B', '0', '1'};
