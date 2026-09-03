#pragma once
#include "packet_types.h"

// This is the ONLY code in the entire Station B project permitted to
// call Serial.print*(). No other module - including radio.cpp - ever
// writes to the serial port. This keeps the LabVIEW-facing stream
// guaranteed free of debug text.
void serial_output_init();

// When radioAvailable is false (the nRF24 chip itself never initialized),
// prints a single "RF: Not Connected" line instead of the telemetry line -
// this is the ONLY status text ever written to this port, and only for
// this one condition. When radioAvailable is true, behavior is unchanged
// from before: exactly the telemetry line, nothing else.
void serial_output_print(const SoilPacket& soil, const CubePacket& cube, bool radioAvailable);
