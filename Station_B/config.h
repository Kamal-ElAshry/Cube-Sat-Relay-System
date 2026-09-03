#pragma once
//
// config.h - Station B hardware & timing configuration.
//
#include <Arduino.h>
#include <stdint.h>

// SPI is fixed hardware on the Nano: MOSI=D11, MISO=D12, SCK=D13.
constexpr uint8_t PIN_RADIO_CE  = 9;
constexpr uint8_t PIN_RADIO_CSN = 10;

// Must match LabVIEW's configured baud rate exactly.
constexpr unsigned long SERIAL_BAUD = 115200UL;

constexpr unsigned long PRINT_INTERVAL_MS = 1000UL;
