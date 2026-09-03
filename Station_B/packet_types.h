#pragma once
//
// packet_types.h
//
// Shared RF protocol definitions. This file is intentionally identical
// across all three projects (Station_A_Soil, CubeSat, Station_B).
// If you change a field here, copy the SAME change into the other two
// projects' packet_types.h, or the three nodes will disagree about the
// wire format.
//
#include <stdint.h>

// ---------------------------------------------------------------------
// Fixed nRF24 transport envelope
// ---------------------------------------------------------------------
// The nRF24L01 hard limit is 32 bytes per payload. This project keeps a
// FIXED 32-byte transport envelope regardless of how large the logical
// packet actually is: every transmit zero-pads up to 32 bytes, and every
// receive only interprets sizeof(the actual struct) bytes starting at
// offset 0. This means the logical packets can grow later (as long as
// they stay <= 32 bytes) without changing the RF layer at all.
constexpr uint8_t RF_PAYLOAD_SIZE = 32;

// ---------------------------------------------------------------------
// RF distribution modes (see radio_config.h for per-link selection)
// ---------------------------------------------------------------------
enum class RfDistributionMode : uint8_t {
    ONE_TO_ONE,             // one destination address, ACK enabled, tracked
    SHARED_MULTI_RECEIVER,  // one shared address, ACK disabled, anonymous listeners
    EXPLICIT_MULTI_RECEIVER // N destination addresses, ACK enabled, each tracked
};

// ---------------------------------------------------------------------
// Packet identification
// ---------------------------------------------------------------------
enum PacketId : uint8_t {
    PACKET_SOIL = 0x01,
    PACKET_CUBE = 0x02,
};

// ---------------------------------------------------------------------
// Soil telemetry (Station A -> CubeSat)
// ---------------------------------------------------------------------
enum RainState : uint8_t {
    RAIN_NONE     = 0,
    RAIN_DETECTED = 1,
};

enum SoilState : uint8_t {
    SOIL_DRY         = 0,
    SOIL_NORMAL      = 1,
    SOIL_OVERWATERED = 2,
};

enum LightState : uint8_t {
    LIGHT_DARK     = 0,
    LIGHT_DETECTED = 1,
};

// statusFlags bits represent SENSOR VALIDITY ONLY. They are independent
// from rainState / soilState / lightState, which are the actual
// environmental reading regardless of whether the sensor is trusted.
constexpr uint8_t SOIL_STATUS_DHT_VALID   = 0x01;
constexpr uint8_t SOIL_STATUS_RAIN_VALID  = 0x02;
constexpr uint8_t SOIL_STATUS_SOIL_VALID  = 0x04;
constexpr uint8_t SOIL_STATUS_LIGHT_VALID = 0x08;

#pragma pack(push, 1)
struct SoilPacket {
    uint8_t  packetId;          // PACKET_SOIL
    int16_t  dhtTempC_x10;      // deg C * 10
    uint16_t dhtHumidity_x10;   // %RH * 10
    uint16_t rainRaw;           // raw ADC, 0-1023
    uint8_t  rainState;         // RainState
    uint16_t soilRaw;           // raw ADC, 0-1023
    uint8_t  soilPct;           // 0-100, clamped
    uint8_t  soilState;         // SoilState
    uint8_t  lightState;        // LightState (digital sensor only, no raw value)
    uint8_t  statusFlags;       // bitmask of SOIL_STATUS_*
};
#pragma pack(pop)

static_assert(sizeof(SoilPacket) <= RF_PAYLOAD_SIZE, "SoilPacket exceeds NRF24 payload limit");

// ---------------------------------------------------------------------
// Cube telemetry (CubeSat -> Station B)
// ---------------------------------------------------------------------
constexpr uint8_t CUBE_STATUS_MPU_VALID = 0x01;
constexpr uint8_t CUBE_STATUS_BMP_VALID = 0x02;

#pragma pack(push, 1)
struct CubePacket {
    uint8_t  packetId;            // PACKET_CUBE
    int16_t  accelX;              // raw MPU6050 register value
    int16_t  accelY;
    int16_t  accelZ;
    int16_t  gyroX;
    int16_t  gyroY;
    int16_t  gyroZ;
    int16_t  bmpTempC_x10;        // deg C * 10
    uint32_t bmpPressurePa;       // Pascals, raw
    int16_t  bmpAltitudeM_x10;    // meters * 10, relative to BMP_REFERENCE_PRESSURE_PA
    uint8_t  statusFlags;         // bitmask of CUBE_STATUS_*
};
#pragma pack(pop)

static_assert(sizeof(CubePacket) <= RF_PAYLOAD_SIZE, "CubePacket exceeds NRF24 payload limit");
