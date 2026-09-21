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
// since MPU6050 @0x68, BMP180 @0x77, and INA219 @0x40 are different addresses).
constexpr uint8_t PIN_RADIO_CE  = 9;
constexpr uint8_t PIN_RADIO_CSN = 10;

constexpr uint8_t MPU6050_I2C_ADDRESS = 0x68;
// BMP180 address (0x77) is fixed by the sensor and handled internally by
// the Adafruit_BMP085 library - not referenced directly in application code.

// INA219 default breakout address. Configurable here if your board is
// strapped to a different address (common alternates: 0x41, 0x44, 0x45).
constexpr uint8_t INA219_I2C_ADDRESS = 0x40;

// Which Adafruit_INA219 calibration preset to apply. This determines the
// measurable current/voltage range AND the assumed shunt resistor value -
// it must match your actual physical breakout board, not be left at
// whatever the library defaults to.
//   RANGE_32V_2A     -> 0.1 ohm shunt, up to 32V / 3.2A (most common breakout default)
//   RANGE_32V_1A     -> 0.1 ohm shunt, up to 32V / 1A, finer resolution
//   RANGE_16V_400MA  -> 0.1 ohm shunt, up to 16V / 400mA, finest resolution
// DEFAULT BELOW ASSUMES THE COMMON 0.1 OHM SHUNT - CONFIRM AGAINST YOUR
// ACTUAL BOARD (check the breakout's silkscreen/datasheet) BEFORE TRUSTING
// READINGS. If your board uses a different shunt value, the Adafruit
// library's fixed presets will not be correct for it and a custom
// calibration (see ina219.cpp) is needed instead.
enum class Ina219Calibration : uint8_t {
    RANGE_32V_2A,
    RANGE_32V_1A,
    RANGE_16V_400MA,
};
constexpr Ina219Calibration INA219_CALIBRATION = Ina219Calibration::RANGE_32V_2A;

// ---------------------------------------------------------------------
// Timing
// ---------------------------------------------------------------------
constexpr unsigned long SENSOR_INTERVAL_MS   = 500UL;  // MPU6050/BMP180/INA219 read cadence
constexpr unsigned long RELAY_TX_INTERVAL_MS = 2000UL; // RX->TX->RX cycle cadence

// ---------------------------------------------------------------------
// Debug status output (bench debugging only, not part of the RF protocol)
// ---------------------------------------------------------------------
// Change freely at any time - independent of the nRF24 configuration.
constexpr unsigned long SERIAL_BAUD             = 9600UL;
constexpr unsigned long STATUS_PRINT_INTERVAL_MS = 1000UL;

// How often a failed BMP180 retries initialization in the background.
constexpr unsigned long BAROMETER_RETRY_INTERVAL_MS = 5000UL;

// How often a failed INA219 retries initialization in the background.
constexpr unsigned long INA219_RETRY_INTERVAL_MS = 5000UL;

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
