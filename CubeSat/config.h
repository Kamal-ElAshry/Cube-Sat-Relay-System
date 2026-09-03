#pragma once
//
// config.h - CubeSat ("Cube") hardware & timing configuration.
//
#include <Arduino.h>
#include <stdint.h>

// ---------------------------------------------------------------------
// Pin assignments
// ---------------------------------------------------------------------
// SPI is fixed hardware on the UNO: MOSI=D11, MISO=D12, SCK=D13.
// I2C is fixed hardware on the UNO: SDA=A4, SCL=A5 (shared bus, no conflict
// since MPU6050 @0x68 and BMP180 @0x77 are different addresses).
constexpr uint8_t PIN_RADIO_CE  = 9;
constexpr uint8_t PIN_RADIO_CSN = 10;

constexpr uint8_t MPU6050_I2C_ADDRESS = 0x68;
// BMP180 address (0x77) is fixed by the sensor and handled internally by
// the Adafruit_BMP085 library - not referenced directly in application code.

// ---------------------------------------------------------------------
// Timing
// ---------------------------------------------------------------------
constexpr unsigned long SENSOR_INTERVAL_MS   = 500UL;  // MPU6050/BMP180 read cadence
constexpr unsigned long RELAY_TX_INTERVAL_MS = 2000UL; // RX->TX->RX cycle cadence

// ---------------------------------------------------------------------
// Debug status output (bench debugging only, not part of the RF protocol)
// ---------------------------------------------------------------------
// Change freely at any time - independent of the nRF24 configuration.
constexpr unsigned long SERIAL_BAUD             = 9600UL;
constexpr unsigned long STATUS_PRINT_INTERVAL_MS = 1000UL;

// How often a failed BMP180 retries initialization in the background.
constexpr unsigned long BAROMETER_RETRY_INTERVAL_MS = 5000UL;

// ---------------------------------------------------------------------
// Scaling
// ---------------------------------------------------------------------
constexpr int16_t SCALE_TEMP_X10     = 10;
constexpr int16_t SCALE_ALTITUDE_X10 = 10;

// ---------------------------------------------------------------------
// Barometric altitude reference
// ---------------------------------------------------------------------
// Sea-level reference pressure used to convert BMP180 pressure into an
// altitude estimate. Set this to your local/current sea-level pressure
// (from a weather service) for an accurate absolute altitude, or leave
// at the standard atmosphere value for a consistent relative reading.
constexpr float BMP_REFERENCE_PRESSURE_PA = 101325.0f;
