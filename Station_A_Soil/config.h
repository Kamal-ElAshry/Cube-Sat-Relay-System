#pragma once
//
// config.h - Station A ("Soil") hardware & calibration configuration.
// All magic numbers used by the application live here, not scattered
// through the .cpp files.
//
#include <Arduino.h>
#include <stdint.h>

// ---------------------------------------------------------------------
// Pin assignments
// ---------------------------------------------------------------------
// SPI is fixed hardware on the Nano: MOSI=D11, MISO=D12, SCK=D13.
constexpr uint8_t PIN_RADIO_CE  = 9;
constexpr uint8_t PIN_RADIO_CSN = 10;

constexpr uint8_t PIN_DHT11     = 2;

constexpr uint8_t PIN_RAIN_AO   = A0;
// Digital rain output intentionally unused: section 5 requires the
// analog reading + configurable threshold to be the source of truth.
constexpr uint8_t PIN_RAIN_DO   = 3;   // reserved for future use, not read

constexpr uint8_t PIN_SOIL_AO   = A1;

// LDR module is DIGITAL-ONLY (verified against product description).
constexpr uint8_t PIN_LIGHT_DO  = 4;

// ---------------------------------------------------------------------
// Timing
// ---------------------------------------------------------------------
constexpr unsigned long SENSOR_INTERVAL_MS  = 1000UL;
constexpr unsigned long RF_SEND_INTERVAL_MS = 2000UL;

// ---------------------------------------------------------------------
// Debug status output (bench debugging only, not part of the RF protocol)
// ---------------------------------------------------------------------
// Change freely at any time - independent of the nRF24 configuration.
constexpr unsigned long SERIAL_BAUD              = 9600UL;
constexpr unsigned long STATUS_PRINT_INTERVAL_MS  = 1000UL;

// An analog sensor whose raw reading stays EXACTLY identical for this
// many consecutive samples is flagged "Not Working" in the status line
// (real analog noise almost never stays bit-identical this long). This
// is a debug-only heuristic - it does not affect statusFlags in the
// transmitted SoilPacket.
constexpr uint16_t STUCK_SAMPLE_THRESHOLD = 30;

// ---------------------------------------------------------------------
// Rain sensor
// ---------------------------------------------------------------------
// Raw ADC threshold used to derive the interpreted rain state.
// VERIFY POLARITY against your physical module before trusting this:
// most resistive raindrop boards read a LOWER analog voltage as MORE
// rain (wetter surface = lower resistance = more current pulled).
constexpr uint16_t RAIN_ADC_THRESHOLD          = 500;
constexpr bool     RAIN_TRIGGERS_BELOW_THRESHOLD = true; // set false if your module is inverted

// ---------------------------------------------------------------------
// Soil moisture calibration
// ---------------------------------------------------------------------
// These MUST be calibrated against your actual physical sensor + soil.
// map() handles either polarity correctly as long as these two values
// reflect what YOUR module actually reads dry vs. wet - do not assume
// 0=0% / 1023=100%.
constexpr uint16_t SOIL_ADC_DRY = 800;  // raw ADC reading in dry air/soil
constexpr uint16_t SOIL_ADC_WET = 300;  // raw ADC reading fully saturated

constexpr uint8_t SOIL_PCT_DRY_THRESHOLD         = 30; // <= this % => Dry
constexpr uint8_t SOIL_PCT_OVERWATERED_THRESHOLD = 85; // >= this % => Overwatered

// ---------------------------------------------------------------------
// Light sensor (digital only)
// ---------------------------------------------------------------------
// The logic level PIN_LIGHT_DO reports when light IS detected.
// VERIFY against your physical module - many comparator boards pull
// this LOW when the measured light exceeds the onboard potentiometer
// threshold, but this varies by board revision.
constexpr uint8_t LDR_ACTIVE_LEVEL = LOW;

// ---------------------------------------------------------------------
// Scaling
// ---------------------------------------------------------------------
constexpr int16_t SCALE_TEMP_X10     = 10;
constexpr int16_t SCALE_HUMIDITY_X10 = 10;
